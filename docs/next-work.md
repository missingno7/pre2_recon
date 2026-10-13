# Next work

Choose by: what would force a translator (or now, a port maintainer) to guess? Addresses:
code offsets (cs:XXXX), ds offsets (ds:XXXX). `python tools/where.py ADDR` finds the line.
The completion status of the port is docs/completeness.md.

## Coverage (separate dimensions)

| Dimension | State |
|---|---|
| Binary | program image, all MZ header fields, relocation set exact; 133/134 relocations in original order (5 header bytes differ; docs/executable.md) |
| Classification | all code decoded as instructions (3 explicit encodings); 60 DATA bytes still `; UNKNOWN` |
| Names | 9 routine placeholders left (dead code); about 225 data placeholders (unproven meanings stay placeholders) |
| Translation | every routine is translated, platform-replaced, an alternative hardware path, an error path or dead (tools/coverage.py: none unresolved) |
| Differential tests | see docs/completeness.md section 11 (all equal; the ES model and, under PRE2_POISON, garbage in unspecified registers included) |
| Oracle execution coverage | tools/coverage.py over a PRE2_COVERAGE run (docs/completeness.md sections 1-2) |
| Platform models | VGA (known-answer tests: tests/_vga_kat.py), BIOS mode set, DOS files, port log; host: PIT, DMA ch1, SB DSP 14h/40h, PIC mask, keyboard (set 1 with E0 prefixes, typematic), no joystick, CRT timeline, period load and VGA bus time |
| Native | SDL3 x64 host; smoke test and four transition scenarios (docs/smoke-test.md) |

## Open items, by risk

1. **Level codes depend on the machine** (ComputePasswordCode: BIOS ROM fold, CPU class).
   The host's ROM area is zero and the CPU class 3: codes are those of such a machine.
   Decide whether to accept the codes of real machines as well (a deliberate extension).
2. **Untested-by-construction branches** (docs/completeness.md section 10): the live
   game-over screen with the Ctrl+Alt+E gate open, the timer exits of the game-over loop
   and the front end (driven by word_27F0 from the timer interrupt, which the bounded tests
   do not run), the "no free tally slot" jump (needs 20 concurrent flights), the world-map
   nodes 1, 3, 5, 6, 7, 8 as full runs. A scripted timer-interrupt delivery on both sides
   (like the button script) would close the timer exits.
3. **Alternative hardware paths** are translated but not presented by the host: AdLib
   sample output and the PC speaker (effects only, no unique content), EMS/XMS resource
   placement (fails loudly), joystick (the host reports none).
4. **Unresolved data facts**: the producer of WarpLevelTable[6] = 0 (no shipped object
   uses it), the consumer of byte_27E9 (keypad 5), the pre-image of the code hash cheat.
5. Relocation order (5 header bytes) and the remaining DATA placeholders: documentation
   quality, no behavioural effect.
