# Next work

Choose by: what would force a translator to guess? Addresses: code offsets (cs:XXXX),
ds offsets (ds:XXXX). `python tools/where.py ADDR` finds the source line.

## Coverage (separate dimensions; update when they change)

| Dimension | State |
|---|---|
| Binary | program image, all MZ header fields and the relocation set exact; 133/134 relocations in original order (5 header bytes differ) |
| Classification | all code decoded as instructions (3 explicit encodings: 2 BIOS reboot far jumps, 1 patched far call); 39.7 KB of DATA still `; UNKNOWN` (much of it level-loaded tables and buffers whose initial bytes are zero) |
| Names | 350 routine/entry labels, 9 placeholders left (dead code); 546 data labels, 225 placeholders |
| Contracts | comment blocks on almost every routine from ten region analyses (static, by workers; spot-checked, not all independently verified) |
| Translation | c/sqz.c (resource loader + 3 codecs): 51/51 shipped files equal, mutants caught. Sound mixer/tracker and sprite pipeline: in progress |
| Platform models | VGA (planar, latches, write/read modes, DAC, CRTC/attribute, retrace), DOS files; missing: keyboard controller, PIT/PIC, SB DSP/DMA, AdLib |
| Native | SDL3 3.4.16 x64 pinned (metadata/sdl3.json); no host code yet |

## Translation-critical uncertainties (from the region reports)

1. Level codes hash machine state (ComputePasswordCode): BiosSeed folds F000:FFF0..FFFF and
   option ROM bytes, rotated by the runtime CPU class (cs:CpuClass, 0/2/3). A port must
   choose both values; document the choice (and whether to accept every code variant).
2. LoadLevel stash passes (3F9C/3FE9) appear to keep only plane 3 of the stashed area; check
   before translating (R4b).
3. Tile graphics address VB + V*256 + 200h + 128*n can pass 64 KiB for large MapRows.
4. Mixer overrun quirk: a looping-sample wrap can write past byte 167 of the block and store
   into instrument records; the C model needs a rule for what DMA has already fetched (R3).
5. Meanings not proven: byte_6BC5 (PlayerAltModeFlag), byte_6BC7, byte_6BE1 gate,
   word_6BE2 (countdown / frame remap), byte_B198 (expert flag copy that disables entities
   with kind bit 7), the 52E8 table, 6BBE ring (popup vs trail), 280Dh (no writer found).
6. Game-over trigger from PollInputState (0DE2: pending scan code during demo playback).
7. PlayerHurtTimer (4F2D) is set to 2Ch and never decremented in the code read by R5b.
8. HUD score DIV 25000 faults at 1638400000 internal points (unreachable in practice).
9. Interrupt delivery model for the host (docs/timing.md): verify per handler that no main
   code read-modify-write depends on interrupt timing.

## Known semantic conflicts between region reports (resolve from the instructions)

- ds:7DE6: R5a "effect particles (x, y, angle, speed)" vs R5c "debris (x, y, kind, phase)".
- ds:6BBE: popup ring (R5a name kept) vs trail ring (R5b).
- word_4F22/word_4F2A: X/Y velocity (R5b, R5d moderate) vs pre2_port docs.

## Order of the remaining work

1. Finish sound and sprite translations; then the renderer (tile ring, scrolling, page
   copy, HUD), the gameplay frame routines in main-loop order, the screens, and the
   interrupt handlers (keyboard, timer tick, SB block).
2. Platform models still missing (keyboard controller 60h/64h, PIT, PIC, SB DSP + DMA).
3. include/RECORDS.INC from the R5c field list (render slot / child, entity, debris,
   L5 projectile) and symbolic field offsets in the source.
4. Native host: memory image from our own build, VGA presentation (mode 0Dh planes, CRTC
   start, DAC), audio from mixer blocks, keyboard scan codes, 72.83 Hz tick model; then the
   bounded smoke test (startup -> first playable frame -> bounded ticks -> pacing check).
