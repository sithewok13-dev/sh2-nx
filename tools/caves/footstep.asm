; EE DoubleFootstepFix (SH2EE Patches/DoubleFootstepFix.cpp), at 60 fps only (RATE60, set by game/controls.c):
; when James's (or Maria's) walk or run animation changes just as the last one loops, the first two footstep
; flags fire in the same frame and the step sounds twice. The first is marked played instead.
; tools/caves/build.sh assembles this into tools/caves/caves.py, which tools/patch_exe.py writes into the exe.
bits 32
org 0x4012C0

RATE60       equ 0x0EC02000      ; byte: 1 while the game runs at 60 fps
FOOTSTEP0    equ 0x01FB8070      ; footstep 0's "played" flag ([esi+0x1FB8070] at 0x54B322)
JAMES_SITE   equ 0x0054B330      ; movsx eax, byte [esp+esi*8+0x11]: replaced by a jump here
MARIA_SITE   equ 0x00549715      ; movsx eax, byte [esp+esi*8+0x09]

james:                           ; 0x4012C0
    cmp byte [RATE60], 0
    je .resume
    mov al, [esp + 0x18]         ; footstep 1's frame
    push eax
    call skip_double
    add esp, 4
    test eax, eax
    jz .resume
    pop esi                      ; the handler's own epilogue: nothing more to play this frame
    pop ebp
    pop ebx
    add esp, 0x1B0
    ret
.resume:
    movsx eax, byte [esp + esi*8 + 0x11]
    jmp JAMES_SITE + 5

maria:
    cmp byte [RATE60], 0
    je .resume
    mov al, [esp + 0x10]
    push eax
    call skip_double
    add esp, 4
    test eax, eax
    jz .resume
    pop esi
    add esp, 0x12C
    ret
.resume:
    movsx eax, byte [esp + esi*8 + 0x09]
    jmp MARIA_SITE + 5

skip_double:                     ; eax 1: footsteps 0 and 1 both fire this frame (footstep 0 marked played)
    test esi, esi
    jnz .no
    mov al, [esp + 4]
    test al, al
    jl .no
    movsx dx, al
    cmp cx, dx
    jl .no
    inc byte [FOOTSTEP0]
    mov eax, 1
    ret
.no:
    xor eax, eax
    ret
