; EE SprayEffect spawn rate (SH2EE Patches/SprayEffect.cpp), at 60 fps only (RATE60): the Lying Figure's and the
; hyper spray's particles are made every other frame, as at 30 fps. Each spawner's `push esi; push 5; push handler`
; becomes a jump here (the stubs; their check, spray_active.asm, is at 0x401980); the 30 fps path (and every other frame at 60) does those pushes and goes on.
bits 32
org 0x401340

SPRAY_FRAME     equ 0x0EC02030   ; dword: the game frame last seen (EE SprayFrameCounter)
SPRAY_ACTIVE    equ 0x0EC02034   ; dword: this frame makes a particle (EE IsSprayActive)
PARTICLE_TABLE  equ 0x01DB96C0   ; the particle lists (EE GetParticleTableAddr)
FRAME_COUNTER   equ 0x00933430   ; the game's frame counter (EE GetFrameCounterAddr)
LYING_SITE      equ 0x004A4A10
LYING_HANDLER   equ 0x004A3EC0
HYPER_SITE      equ 0x004A5090
HYPER_HANDLER   equ 0x004A43F0
active          equ 0x00401980   ; spray_active.asm

lying:
    call active
    test eax, eax
    jnz .go
    ret                          ; no particle this frame
.go:
    push esi
    push 5
    push LYING_HANDLER
    jmp LYING_SITE + 8

hyper:
    call active
    test eax, eax
    jnz .go
    ret
.go:
    push esi
    push 5
    push HYPER_HANDLER
    jmp HYPER_SITE + 8
