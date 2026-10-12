; Record layouts (field offsets, sizes) recovered from the object, player and 
; render code.
; Constants only; the arrays are labels in DATA (src/MODULE1.ASM).

; Render slot / child record: RenderSlots (ds:4F0A), 116 slots of RS_SIZE byte
; s.
; Slot 0 = player display sprite, slot 1 = player record (PlayerX), 4FD0h.. = 
; 12 child
; slots (AllocChildRecord), 50A8h..52E7h = 32 particle slots.
RS_SIZE         equ 12h
RS_X            equ 00h     ; word: world X, pixels
RS_Y            equ 02h     ; word: world Y, pixels (bottom anchor)
; word: bits 0-12 frame, 2000h drawn, 4000h flash, 8000h mirror; FFFFh = emp
; ty
RS_FRAME        equ 04h
RS_FLAGS        equ 05h     ; byte: high byte of RS_FRAME
; word: child: owning entity record; particle: X velocity
RS_OWNER        equ 06h
; word: child X velocity, 1/16 px per frame; FFFFh = none
RS_XVEL         equ 08h
RS_FACING       equ 09h     ; byte: bit 80h facing, copied to RS_FRAME bit 15
RS_YVEL         equ 0Ah     ; word: child Y velocity, 1/16 px per frame
; word: child script cursor; particle: lifetime countdown
RS_SCRIPT       equ 0Ch
; byte: child state (0FFh dying, >= 0Ah far-cull death); particle: Y velocit
; y word
RS_STATE        equ 0Eh
; byte: child parameter copied from entity ENT_FLAGS+1 at spawn
RS_AUX          equ 0Fh
RS_COUNT        equ 10h     ; byte: hit / accumulator
; byte: blink countdown (drawn every 4th tick while nonzero)
RS_BLINK        equ 11h
RENDER_SLOTS    equ 116
CHILD_SLOTS     equ 12
PARTICLE_SLOTS  equ 32

; Entity record (second-pass list at ds:8489, 2048 bytes, variable stride; a s
; tride byte
; >= 32h ends the list).
ENT_STRIDE      equ 00h     ; byte: record size
; byte: bits 0-6 EntityHandlerTable index; bit 7 disables unless byte_B198 =
;  1
ENT_KIND        equ 01h
; word: sprite reference copied to the child frame; FFFFh = retired
ENT_SPRITE      equ 02h
; byte: 01 dying child may fall, 02 keep sprite, 04 child busy, 08 terrain
ENT_FLAGS       equ 04h
ENT_PERIOD      equ 06h     ; byte: TickEntityTimer period
ENT_TIMER       equ 07h     ; byte: saturating timer
; kinds 0/10: tile box x, y, w, h bytes; kinds 1..12: pixel X word, Y word a
; t 0Bh
ENT_BOX         equ 09h
ENT_PARAM       equ 0Dh     ; 0Dh..13h: per-kind child parameters
ENT_END_STRIDE  equ 32h

; Debris record: DebrisRecord (ds:7DE6), 20 records.
DEBRIS_SIZE     equ 6
DEBRIS_X        equ 00h     ; word: X, FFFFh = free
DEBRIS_Y        equ 02h     ; word
DEBRIS_KIND     equ 04h     ; byte: kind passed by the spawner
DEBRIS_PHASE    equ 05h     ; byte
DEBRIS_COUNT    equ 20

; Level-5 projectile: L5Projectile (ds:7DAF), 5 records.
L5P_SIZE        equ 0Bh
L5P_X           equ 00h     ; word
L5P_Y           equ 02h     ; word
L5P_FRAME       equ 04h     ; word: FFFFh = free
L5P_YVEL        equ 06h     ; word: >> 4 per frame
L5P_DRIFT       equ 08h     ; word
L5P_DRIFT_STEP  equ 0Ah     ; byte
L5P_COUNT       equ 5
