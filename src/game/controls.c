/* CONTROLS (the touch tray, touch.c): modern controls by default, the original ones as CLASSIC.
 * The game's control type is the byte at 0x1DBC008 (SH2EE's GetControlTypePointer: the operand of the
 * `mov al, [..]` 0x16 bytes past 0x4676E9): 0 rotational (the original "3D" tank controls), 1 directional
 * (the "2D" type: the stick points where James goes, from the camera's view). The tray's choice is kept in
 * touch.cfg and held every frame, so the game's own options menu (or a save's settings) can't undo it. */
#include "../runtime/rt.h"

#define CONTROL_TYPE 0x01DBC008u

static int classic, chosen;  /* chosen: the touch tray decides (iOS); elsewhere the game's own setting stands */

void controls_set_classic(int on) {
    classic = !!on;
    chosen = 1;
    rt_log("controls: %s", classic ? "classic (rotational)" : "modern (directional)");
}

/* Game thread, once per presented frame (d3d8.c). */
static void frame_rate_tick(void);
void controls_tick(void) {
    frame_rate_tick();
    if (!chosen) return;
    uint8_t want = classic ? 0 : 1;
    if (MEM8(CONTROL_TYPE) != want) MEM8(CONTROL_TYPE) = want;
}

#define FLASHLIGHT_HAD 0x01F7A7E0u  /* SH2EE's GetFlashLightAcquiredPointer: the inventory's bits, 0x40000 the flashlight (item 18) */
/* The flashlight's switch, for the touch overlay's LIGHT picture: the byte at 0x1F7A806 (SH2EE's
 * GetFlashlightSwitchPointer: 0x29 bytes past the pattern at 0x43ED25), flipped by the light's button
 * (sub_0048E800) and set at a new game (sub_0049EE10). */
int game_flashlight_on(void) {
    if (!(MEM32(FLASHLIGHT_HAD) & 0x40000)) return 0;  /* not had yet: the switch is on from a new game anyway */
    return MEM8(0x01F7A806u) ? 2 : 1;
}

/* ---- which screen the game is on, for the touch overlay (touch.c) ----
 * The addresses are SH2EE's (Patches/Common.cpp), found in this exe by the same byte patterns:
 *   0x932178  the event index (GetEventIndexPointer): 4 in play, 15 a movie, 16 the pause menu, 2 the main menu,
 *             5 the map, 6 the inventory, 7 options, 8 memos, 9 saving, 0 and 1 loading, 10-13 results and endings
 *   0x1F7A7C4 the cutscene playing, 0 for none (GetCutsceneIDPointer)
 *   0x932030  the pause menu's quit question is up (GetQuitSubmenuFlagPointer)
 *   PAUSE_ROW the pause menu's highlighted row (GetPauseMenuButtonIndexPointer)
 * The pause menu's rows, in the game's 480-high screen: from 216, 30 apart, Resume to Quit (SH2EE's hitboxes). */
#define EVENT_INDEX 0x00932178u
#define CUTSCENE_ID 0x01F7A7C4u
#define QUIT_FLAG   0x00932030u
#define TRANSITION  0x01F7D490u  /* fading (GetTransitionStatePointer: 0 none, 1 to black, 2 black, 3 from black) */
#define PAUSE_ROW   0x009326B4u  /* the operand of the `mov eax, [..]` 0x10 past the pattern at 0x407497 */

/* James is somewhere in the game (not the title, its menus or the opening): a room is loaded (SH2EE's
 * GetRoomIDPointer, 0 for none) and the screen isn't the main menu. */
#define ROOM_ID     0x01FB7DACu  /* the `cmp dword [..]` 0xD7 past the pattern at 0x52A4A0 */
int game_in_world(void) { return MEM32(ROOM_ID) != 0 && MEM8(EVENT_INDEX) != 2; }

int game_screen(void) {
    if (MEM32(TRANSITION)) return SCREEN_LOADING;  /* fading out, black, or fading in: nothing to touch yet */
    switch (MEM8(EVENT_INDEX)) {
    case 4: return MEM32(CUTSCENE_ID) ? SCREEN_CUTSCENE : SCREEN_PLAY;
    case 15: return SCREEN_CUTSCENE;
    case 16: return SCREEN_PAUSE;
    case 0: case 1: return SCREEN_LOADING;
    case 5: case 6: case 8: return SCREEN_GAME_MENU;
    default: return SCREEN_MENU;
    }
}

/* A tap on the pause menu, y in the game's 480-high screen: picks the row under it and says to confirm (1), or
 * nothing (0). The quit question (yes / no) is left to the arrows and OK. */
int game_pause_tap(float y480) {
    if (MEM8(QUIT_FLAG)) return 0;
    int row = (int)((y480 - 216) / 30);
    if (y480 < 216 || row > 4) return 0;
    MEM8(PAUSE_ROW) = (uint8_t)row;
    return 1;
}

/* ---- the frame rate (the touch tray's RATE: 60 by default on iOS, the game's 30 elsewhere) ----
 * The game steps its logic in 60 Hz ticks; its frame limiter (sub_004F6CE0) waits until the interval at
 * 0xA33350 (64-bit, in QueryPerformanceCounter ticks: the frequency is at 0xA33348) has passed since the last
 * frame, then counts the ticks the frame took (0xA3336C, two at 30 fps, one at 60). The interval is set to a
 * thirtieth of a second when the game starts (sub_004F6C80); a sixtieth gives 60 frames a second. */
#define QPC_FREQ        0x00A33348u
#define FRAME_INTERVAL  0x00A33350u
static int frame_rate;  /* 0: leave the game's own */

void game_set_frame_rate(int fps) { frame_rate = fps == 60 || fps == 30 ? fps : 0; }

/* The Enhanced Edition's 60 fps fixes that are speeds in the game's data (SH2EE Patches/SixtyFPSPatch.cpp, the
 * addresses found by its byte patterns in this exe): at 60 fps EE's value, at 30 the game's own (kept as the
 * game starts). The rest (fog, bullet shells, timers...) are hooks in the code. */
static const struct { uint32_t va; float sixty; const char *what; } speeds[] = {
    {0x006BC194u, 0.0166665f, "water"},                      /* GetWaterAnimationSpeedPointer */
    {0x0070AE40u, 0.00027777222565f, "staircase flames"},    /* GetStaircaseFlamesLightingPointer */
    {0x0089C1A4u, 20.0f, "water level lowering"},            /* GetWaterLevelLoweringStepsPointer */
    {0x006BC1F4u, 5.333333492f, "water level rising"},       /* GetWaterLevelRisingStepsPointer */
    {0x006C10A8u, 0.093333332985f, "bug room flashlight"},   /* GetBugRoomFlashlightFixPointer */
};
static float speeds_orig[sizeof speeds / sizeof *speeds];
static int speeds_kept, speeds_rate;

static void frame_rate_tick(void) {
    if (!frame_rate) return;
    uint64_t freq = (uint64_t)MEM32(QPC_FREQ) | (uint64_t)MEM32(QPC_FREQ + 4) << 32;
    if (!freq) return;  /* not started yet */
    uint64_t want = freq / frame_rate;
    MEM32(FRAME_INTERVAL) = (uint32_t)want;
    MEM32(FRAME_INTERVAL + 4) = (uint32_t)(want >> 32);
    if (!speeds_kept) {
        for (unsigned i = 0; i < sizeof speeds / sizeof *speeds; i++) speeds_orig[i] = MEMF(speeds[i].va);
        speeds_kept = 1;
    }
    if (speeds_rate != frame_rate) {
        for (unsigned i = 0; i < sizeof speeds / sizeof *speeds; i++)
            MEMF(speeds[i].va) = frame_rate == 60 ? speeds[i].sixty : speeds_orig[i];
        rt_log("controls: %d fps", frame_rate);
        speeds_rate = frame_rate;
    }
}

int game_frame_rate(void) { return frame_rate ? frame_rate : 30; }  /* for the code hooks (ee.c) */
