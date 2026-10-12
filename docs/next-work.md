# Next work

Choose by: what would force a translator (or now, a port maintainer) to guess? Addresses:
code offsets (cs:XXXX), ds offsets (ds:XXXX). `python tools/where.py ADDR` finds the line.

## Coverage (separate dimensions)

| Dimension | State |
|---|---|
| Binary | program image, all MZ header fields, relocation set exact; 133/134 relocations in original order (5 header bytes differ; docs/executable.md) |
| Classification | all code decoded as instructions (3 explicit encodings); 60 DATA bytes still `; UNKNOWN` |
| Names | 9 routine placeholders left (dead code); about 225 data placeholders (unproven meanings stay placeholders) |
| Contracts | comment blocks on almost every routine; corrected where the translation proved them wrong |
| Translation | every routine on the game's live paths is in c/ (sqz, sound, sprites, render, system, startup, credits, interrupts, level, player, objects, effects, screens, game); 2603 differential cases in 14 suites, all equal; mutants caught in every suite |
| Platform models | VGA, DOS files, port log; host: PIT, DMA ch1, SB DSP 14h/40h, PIC mask, keyboard, no joystick |
| Native | SDL3 x64 host builds (tools/hostbuild.py); smoke test reaches level 1 with 72.85 Hz ticks, 24.27 frames/s, 3.00 ticks/frame, 50.05 SB blocks/s (docs/smoke-test.md) |

## Open items, by risk

1. **Render-slot fill ES on warp/bonus restarts.** loc_01C4's `rep stosw` uses the ES the
   code before it left. Established: A000h on the CurrentLevel < 0Ah path. Not established:
   CurrentLevel >= 0Ah after RunFrontEnd (A000h or DATA by path) and after UpdateLevelState's
   CF exit (tokens A000h / DATA / LevelSegment / unknown, from the child-handler tables).
   c/game.c fails with GAME_UNSUPPORTED rather than guess, so the native game stops when a
   warp leads to a level >= 0Ah. Next step: per-path exit-ES tracking in RunFrontEnd and
   UpdateLevelState (extend tools/esflow.py with context, the auditor's esctx approach),
   then pass the established value.
2. **Interrupt delivery points.** The host delivers interrupts at polling points only.
   Every wait loop that waits for interrupt-driven state must poll; the smoke test found one
   that did not (WaitForButtonPressRelease). Audit the remaining loops in c/ against the
   ASM (loops whose exit reads KeyDownTable, LastScanCode, TimerTickCount, word_27F0,
   SbIrqCount...).
3. **Level codes depend on the machine** (ComputePasswordCode: BIOS ROM fold, CPU class).
   The host's ROM area is zero and the CPU class 3: codes are those of such a machine.
   Decide whether to accept the codes of real machines as well (a deliberate extension).
4. **Untranslated or boundary paths**, all failing loudly: EMS/XMS resource placement and
   paging (the host configures neither), DSP time-outs and the "not enough memory" message
   paths, the ESC quit wipe (the host exits), AdLib sample output and PC speaker sequences
   (translated, not exercised by the host, which configures a Sound Blaster).
5. **Register-contract audit by tool.** tools/livein.py lists live-in register candidates
   per routine (conservative); adjudicate them against the C signatures as tools/esflow.py
   did for ES (one real mismatch found and fixed: the level-end tally clear).
6. Relocation order (5 header bytes) and the remaining DATA placeholders: documentation
   quality, no behavioural effect.
