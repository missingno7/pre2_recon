; Record layouts recovered from the object, player and render code.
; Constants only; the arrays are labels in DATA (src/MODULE1.ASM).
; Keep lines short: TASM wraps long listing rows (tools/lst.py).

; Render slot / child record: RenderSlots (ds:4F0A), 116 slots.
; Slot 0 = player display sprite, slot 1 = player record (PlayerX);
; 4FD0h.. = 12 child slots, 50A8h..52E7h = 32 particle slots.
RS_SIZE         equ 12h
RS_X            equ 00h     ; word: world X, pixels
RS_Y            equ 02h     ; word: world Y, pixels, bottom anchor
RS_FRAME        equ 04h     ; word: frame | 2000h drawn | 4000h flash
                            ; | 8000h mirror; 0FFFFh = empty slot
RS_FLAGS        equ 05h     ; byte: high byte of RS_FRAME
RS_OWNER        equ 06h     ; word: child: owner entity; particle: X vel
RS_XVEL         equ 08h     ; word: child X vel, 1/16 px; 0FFFFh = none
RS_FACING       equ 09h     ; byte: 80h facing (to RS_FRAME bit 15)
RS_YVEL         equ 0Ah     ; word: child Y velocity, 1/16 px
RS_SCRIPT       equ 0Ch     ; word: child script; particle: lifetime
RS_STATE        equ 0Eh     ; byte: child state, 0FFh = dying
RS_AUX          equ 0Fh     ; byte: child parameter from the entity
RS_COUNT        equ 10h     ; byte: hit / accumulator
RS_BLINK        equ 11h     ; byte: blink countdown
RENDER_SLOTS    equ 116
CHILD_SLOTS     equ 12
PARTICLE_SLOTS  equ 32

; Entity record: list at ds:8489 (2048 bytes), variable stride;
; a stride byte >= ENT_END_STRIDE ends the list.
ENT_STRIDE      equ 00h     ; byte: record size
ENT_KIND        equ 01h     ; byte: handler index, 80h = expert only
ENT_SPRITE      equ 02h     ; word: child frame; 0FFFFh = retired
ENT_FLAGS       equ 04h     ; byte: 01 fall, 02 keep, 04 busy, 08 terrain
ENT_PERIOD      equ 06h     ; byte: timer period
ENT_TIMER       equ 07h     ; byte: saturating timer
ENT_BOX         equ 09h     ; tile box bytes or pixel X/Y words
ENT_PARAM       equ 0Dh     ; 0Dh..13h: per-kind parameters
ENT_END_STRIDE  equ 32h

; Debris record: DebrisRecord (ds:7DE6), 20 records.
DEBRIS_SIZE     equ 6
DEBRIS_X        equ 00h     ; word: X, 0FFFFh = free
DEBRIS_Y        equ 02h     ; word
DEBRIS_KIND     equ 04h     ; byte: kind passed by the spawner
DEBRIS_PHASE    equ 05h     ; byte
DEBRIS_COUNT    equ 20

; Level-5 projectile: L5Projectile (ds:7DAF), 5 records.
L5P_SIZE        equ 0Bh
L5P_X           equ 00h     ; word
L5P_Y           equ 02h     ; word
L5P_FRAME       equ 04h     ; word: 0FFFFh = free
L5P_YVEL        equ 06h     ; word: >> 4 per frame
L5P_DRIFT       equ 08h     ; word
L5P_DRIFT_STEP  equ 0Ah     ; byte
L5P_COUNT       equ 5
