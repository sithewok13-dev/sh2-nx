; EE IsSprayAllocActiveASM, for spray.asm's stubs: eax 1 on every other game frame, or while no particle exists;
; always 1 at 30 fps (RATE60 0).
bits 32
org 0x401980

RATE60          equ 0x0EC02000
SPRAY_FRAME     equ 0x0EC02030   ; dword: the game frame last seen (EE SprayFrameCounter)
SPRAY_ACTIVE    equ 0x0EC02034   ; dword: this frame makes a particle (EE IsSprayActive)
PARTICLE_TABLE  equ 0x01DB96C0   ; the particle lists (EE GetParticleTableAddr)
FRAME_COUNTER   equ 0x00933430   ; the game's frame counter (EE GetFrameCounterAddr)

active:                          ; EE IsSprayAllocActiveASM: eax 1 on every other frame, or with no particle yet
    cmp byte [RATE60], 0
    je .yes                      ; 30 fps: every frame, as the game always did
    mov edx, PARTICLE_TABLE
    mov eax, [edx + 5*4]
    mov edx, [eax + 4]
    cmp eax, edx                 ; list 5 empty?
    jnz .frame
    mov dword [SPRAY_ACTIVE], 1
    jmp .yes
.frame:
    mov eax, [FRAME_COUNTER]
    cmp eax, [SPRAY_FRAME]
    jz .same
    mov [SPRAY_FRAME], eax
    mov eax, [SPRAY_ACTIVE]
    xor eax, 1
    mov [SPRAY_ACTIVE], eax
    jmp .check
.same:
    mov eax, [SPRAY_ACTIVE]
    test eax, eax
.check:
    jnz .yes
    mov eax, 0
    ret
.yes:
    mov eax, 1
    ret
