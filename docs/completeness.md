# Native port completeness

What the native port contains of the original, how strongly each part is evidenced, and
what is left. Statuses: VERIFIED (a bounded ASM-vs-C differential case exercises the
branch), PARTIALLY_VERIFIED, IMPLEMENTED_UNTESTED, INTENTIONALLY_UNSUPPORTED, BLOCKED,
UNRESOLVED. Completeness (everything is translated) and evidence (what the tests prove)
are reported separately. Detailed evidence: build/audit/content.md (content and
reachability), build/audit/sound_dos.md (sound, memory, DOS), build/audit/polling.md
(interrupt-dependent loops); regenerate the routine table with
`PRE2_COVERAGE=1 python tools/difftest.py` and `python tools/coverage.py` (build/coverage.md).

## 1. Original routine coverage

The 308 callable entries code discovery finds (call targets, pointer tables, vectors) are
classified by tools/coverage.py from the source and a small exception table:

| Status | Routines |
|---|---|
| TESTED (every instruction executed by the oracle in some differential case) | 259 |
| PARTIAL (translated; some instructions not executed) | 21 |
| UNTESTED (translated, never executed in a case) | 1 (CalibrateJoystick: the joystick path) |
| INLINE (translated inside another function) | 7 |
| PLATFORM (replaced by a platform service or configuration) | 10 |
| ALTERNATIVE (alternative hardware path, not taken natively) | 3 |
| ERROR_PATH (DOS critical-error handling) | 2 |
| DEAD (no caller) | 5 |
| total | 308 |

Oracle execution over all differential cases: 16870 of 18726 routine instructions (90.1 %),
measured 2026-10-13 (before this pass: 82.9 %, 199 routines fully).

No routine is UNTRANSLATED or UNRESOLVED. The routines without a same-named C function
are translated inline in their callers (the SQZ decoders' helpers, the effect dispatch
default), replaced by platform services or configuration (the CPU, AdLib and Sound
Blaster probes), alternative hardware paths (EMS/XMS resource placement and the
memory-mode decoder), DOS critical-error handling the native services never raise, or
dead code with no caller.

## 2. Reachable-code coverage (oracle execution in the differential tests)

The differential tests run the original routines in Unicorn and record every executed
basic block (PRE2_COVERAGE); build/coverage.md lists each routine's unexecuted ranges.
Every gameplay, rendering, screen and sound routine is fully executed. The 21 partial
routines are partial only through paths outside the native configuration or never
reachable:

| Unexecuted part | Routines | Reason |
|---|---|---|
| the ESC quit wipe tail shared by the interrupt handlers (cs:185C-1971) | SampleTimerIsr, SpeakerTimerIsr, KeyboardIsr | the host exits on ESC (section 14) |
| EMS/XMS modes, memory-mode decoding | PageInResource, InitResourceMemory, LoadSqzFile, LoadRawFile | alternative hardware (section 8) |
| DOS read/open error tails, DSP time-outs | LoadSqzFile, LoadRawFile, DspWriteChecked, LinearDmaAddress | error paths (section 10) |
| joystick | CalibrateJoystick, ReadJoystickTimes, PollInputState (joystick directions) | the host reports no joystick |
| no-VGA probe failure | CheckVgaAdapter, ProbeVideoMemory | ends in a halt |
| the 3BAh (monochrome) retrace branch | WaitVerticalRetrace | OptionVByte is never 0 |
| the startup sequence as one run | ProgramEntry | each step verified separately (section 10) |
| re-tests of decided conditions; the tally-slot overflow | UpdateVerticalCamera, ProcessPlayerContacts, UpdateLevelState | dead / believed unreachable (section 10) |
| the StartSbBlock call with sound on | ActivateResourceSlot (cs:0478) | the cases run with sound off; StartSbBlock is verified on its own and runs natively at every music change |
| timer-driven exit; unused code | ShowCreditsTimed, unused_66D3 | no caller |

## 3. Original-content feature matrix

| Feature | Original evidence | Native implementation | Verification | Status |
|---|---|---|---|---|
| Late-year credits (year >= 1996) | ShowLateYearCredits, ReadClockYear | credits.c | credits suite (1995/1996/2010, BCD edge years) | VERIFIED |
| TITUS, PRESENT title | ShowTitusScreen, ShowPresentTitle | screens.c | screens suite | VERIFIED |
| Front end, MODE (beginner/expert) | RunFrontEnd, RunCallbackMapScreen, UpdateModeSelectFrame | screens.c | screens suite (keys, exit tail) | VERIFIED |
| ENTER CODE, passwords, hash cheat | UpdateCodeEntryFrame, ComputePasswordCode | screens.c, effects.c | 145 scan-code cases; accepted/cancelled tails | VERIFIED (cheat: synthetic history) |
| Attract mode (intro scene, demo) | front_end_loop timer path, RunLevelIntroScene, PollInputState playback | screens.c, system.c | demo start end to end; playback frames; abort by key | VERIFIED |
| World map scroll-in | ShowWorldMapScrollIn | screens.c | levels 0, 2, 4, 9 + composed restarts | PARTIALLY_VERIFIED (6 of 10 nodes by table only) |
| Level start, all 16 levels | LevelRestart, LevelStart, LoadLevel | game.c, render.c | level start of every level 0..0Fh, 3 frames each | VERIFIED |
| Warps into bonus rooms 0Ah..0Ch, 0Eh; returns | UpdateLevelState mode 2, WarpLevelTable | level.c, game.c | composed frame -> restart -> level start | VERIFIED |
| Level exit sequence (iris, walk, tally) | LevelTransitionStart, ItemTallyPath | level.c | level suite + composed bonus exit | VERIFIED |
| Checkpoint restart, death bounce | RespawnPath, RestartFromCheckpoint, PlayDeathBounce | level.c | level suite + composed restarts | VERIFIED |
| Game over (incl. bonus level, demo) | LifeLostPath, GameOverPath, RunGameOverScreen | level.c, screens.c | composed frame -> game over -> front end | VERIFIED |
| Beginner end: CASTLE picture | LevelRestart (level 8/9, byte_B198 = 0) | game.c | composed exit of level 7 -> CASTLE -> front end | VERIFIED |
| Ending (THEEND, intro picture by year) | EndSignal FFh (level 0Eh out of map), ShowEndingPicture, ShowIntroPicture | level.c, credits.c | composed ending -> front end | VERIFIED |
| Secret tiles, cave passages, proximity triggers | ApplySecretTilePatches, TriggerCellChangeGroup, TryCaveTrigger, UpdateProximityTriggers | render.c, effects.c, level.c | effects/level suites (cave scrolling from distant views with the shipped records, the level 5 setup) | VERIFIED |
| BONUS letters, key items, transform timer, checkpoints, weapons 0..3, alt mode | ProcessPlayerContacts tables, sub_67D7 | effects.c, player.c | effects/player suites | VERIFIED |
| Bosses (levels 2, 8), level 5 sequence, level 9 script and finale | UpdateBossFight, UpdateLevel5Sequence, RunLevel9Script, EmitLevel9Sparkles | objects.c | objects suite: real level data (boss positions and energies 64h/32h), all phases, the shipped level 9 script, the finale chain sparks -> contact -> LevelEndMode FFh | VERIFIED |
| Pause (P), give up (F2), sound toggle (F3), credits (Ctrl+Alt+W), intro picture (Ctrl+Alt+E) | WaitFrameSyncFlag, UpdatePlayer, KeyboardIsr, TryShowCredits, TryShowIntroKeyed | player.c, system.c, credits.c | player/system/credits suites (scripted P press/release) | VERIFIED |
| Lives > 99 self-modifying IRET | IncrementPlayerLives | effects.c | player suite | VERIFIED |
| Command-line switches /F /M /A /E /X /L /D /B /V | ProgramEntry loc_0016..loc_007E | game.c parse_options | 18 command-tail cases | VERIFIED |
| Music (12 TRK modules) and effects (SAMPLE.SQZ) on Sound Blaster | SbIrqHandler, TrackerTick, MixChannel, StartSoundEffect | sound.c, interrupts.c | sound/interrupts suites; host smoke test (50.1 blocks/s) | VERIFIED |

## 4. Levels

CurrentLevel 0..0Fh (LEVEL1..9, LEVELA..LEVELG; LEVELH/LEVELI are pictures). 0..9 are
the stages (8 and 9 expert only: beginner mode ends with the CASTLE picture after level 7);
0Ah, 0Bh, 0Ch are bonus rooms reached by warp objects in levels 3, 2, 1; 0Dh and 0Fh
continue stages 3 and 7 (their exits are remapped); 0Eh is the final run, entered by the
level 9 finale warp, and its out-of-map exit is the ending. Codes select stages 0..9 only.
Every level is loaded and started through LevelStart and runs three frames in the
differential tests; the transitions between them (warp in, bonus exit, warp return, the
remapped exits, checkpoint and game-over restarts, the ending) run composed from a frame
through LevelRestart to the next first frame.

## 5. Screens

Every original screen routine is translated: late-year credits, TITUS, PRESENT, front end,
MODE, ENTER CODE, intro scene, attract demo, world map, level exit/tally, game over,
CASTLE, THEEND ending, intro picture (keyed and dated), in-game credits. ShowCreditsTimed
has no caller (dead, translated and tested anyway).

## 6. Assets

All 61 supplied files are accounted for (build/audit/content.md section 4): the startup
files, 16 level and 6 backdrop files, 18 resource-slot files (12 music modules, 6
pictures), sprites, samples, CASTLE, THEEND, LEVELH/LEVELI, and the build inputs. No
referenced asset is missing; no game content lives only in an alternative path. Palettes,
tables and texts embedded in the program are part of the exact program image.

## 7. Supported native configuration

386-class CPU, 640 KiB conventional memory, Sound Blaster at 220h, IRQ 7, DMA 1, no EMS,
no XMS, no joystick, drive C:, RTC from the host clock, BIOS ROM fold zero (level codes
are those of such a machine). 640 KiB holds every level's resources (worst case about
71 KiB spare); every resource is read from disk and decoded on each use, as on the
original without EMS/XMS. The host gives loads and VGA accesses period time
(docs/smoke-test.md), delivers interrupts at service points (docs/timing.md) and scans
the VGA out from register state on a CRT timeline.

## 8. Historical alternative hardware modes

| Path | Original behaviour | Status |
|---|---|---|
| AdLib | sample playback of the SAMPLE.SQZ effects through OPL register 40h at 8403 Hz; no music, no FM | INTENTIONALLY_UNSUPPORTED (translated ISR, not presented by the host; no unique content) |
| PC speaker | 13 frequency-sweep effects; no music | INTENTIONALLY_UNSUPPORTED (translated, no unique content) |
| EMS / XMS resource placement | resources kept in expanded/extended memory | INTENTIONALLY_UNSUPPORTED (fails loudly if configured; unreachable natively) |
| Joystick | calibration and reading through port 201h | INTENTIONALLY_UNSUPPORTED (the host reports no joystick; the code is translated) |

## 9. Resolved unsupported paths (this pass)

- The render-slot fill at loc_01C4 used an unknown ES on every CurrentLevel >= 0Ah path
  (warps into bonus rooms and the final level): the native game stopped there. ES is now
  a register model (pre2_es) that every translated routine leaves as the original leaves
  ES (60 ES-changing routines, all their save/restore pairs); the differential tests
  compare it with the oracle's ES after every case.
- PLAYER_UNSUPPORTED in ordinary play: the hurt state in alt mode calls the text 'LE'
  read past AltStateHandlers (code word 454Ch, a bare `ret`): now the no-op it is.
- The pause key hung the native game (no service point in WaitFrameSyncFlag).
- BuildDecorList compared one pair past its list (a stale word after a previous level
  could be swapped in and its target patched): found by a composed bonus-exit case.
- MODE screen and map scroll-in skipped by the key that started them (no load time), map
  scroll jumping (start address not latched), MODE background too fast (no bus time):
  host timing models (docs/smoke-test.md).

## 10. Remaining unsupported or unverified cases

| Case | Status | Missing evidence / reason |
|---|---|---|
| ProgramEntry as one composed run (startup to the first LevelRestart) | PARTIALLY_VERIFIED | every step is verified in its suite (startup, credits, screens, system) and the whole sequence runs natively in every smoke test; a composed differential run would need the oracle's real hardware probes (SB detection, CPU class) to match the C's platform configuration |
| Timer-driven exits: the game-over loop (word_27F0 >= 276h), the front end's attract timer, ShowCreditsTimed | PARTIALLY_VERIFIED | the bounded tests do not run the timer interrupt; the key exits and the demo start (word_27F0 preset) are verified, the timer increment itself is verified in the interrupts suite; natively the smoke test reaches the attract mode by the timer |
| Live game-over screen with the Ctrl+Alt+E intro gate open | PARTIALLY_VERIFIED | its loop ends only on the timer or a held fire key, which would also hold the intro picture's release wait; the gate is verified in demo mode and with the gate closed |
| UpdateLevelState 4EEB (no free tally slot) | UNRESOLVED (believed unreachable) | needs 20 concurrent tally flights; the exit sequence clears the slots first, one flight starts per 8 ticks and lasts about 27 frames |
| CheckVgaAdapter / ProbeVideoMemory failure path (no VGA: message and halt) | INTENTIONALLY_UNSUPPORTED | the native machine has a VGA; the path ends in an endless loop, not a return |
| UpdateGameOverSprites bubble sort with two equal Y values | quirk preserved, untestable | the original's swap test (next >= current) restarts forever on equal keys; the C keeps the same loop; the shipped game-over sprites do not hit it (the screen runs in every game over) |
| UpdateVerticalCamera 570A, ProcessPlayerContacts 832A | DEAD | re-tests of a condition already decided (the branch before always jumps) |
| World-map nodes 1, 3, 5, 6, 7, 8 as full scroll-in runs | PARTIALLY_VERIFIED | the node table lookup is the same code path as the verified nodes 0, 2, 4, 9 |
| Error tails (DOS read/open errors, fatal halts, DSP time-outs, "not enough memory") | INTENTIONALLY_UNSUPPORTED | the native services do not fail; the original paths stop with a message; the C stops loudly at the same boundary |
| EMS/XMS, AdLib output, PC speaker output, joystick | INTENTIONALLY_UNSUPPORTED | alternative hardware (section 8); no unique content |
| WarpLevelTable[6] = 0, byte_27E9 (keypad 5) consumer, the code hash cheat pre-image | UNRESOLVED (data facts) | no producer/consumer found in code or shipped data; behaviour is translated either way |

## 11. Differential test totals

Final gate, 2026-10-13: `python tools/verify.py` exact; `python tools/difftest.py`
5819/5819 cases equal (credits 55, effects 597, game 70, interrupts 44, level 261, objects
3142, player 463, render 215, screens 261, sound 203, sprites 271, sqz 51, startup 48,
system 138; before this pass: 2603). Every case also compares the ES register model. The
same cases pass with PRE2_POISON=1 (every register a case does not specify starts as
garbage, and CF/ZF set, on both sides), so no routine depends on a register its C contract
does not carry. `python tests/_vga_kat.py`: all VGA known-answer checks pass.

## 12. Mutation-test results

Every new case group was checked with deliberate mutations of the C it covers (one at a
time, restored afterwards); all were caught except the equivalent mutants listed by the
workers. Highlights: every SET_ES site and save/restore pair of the ES model (removing any
one makes the ES comparison fail; two sites are indistinguishable by design: both load the
same segment); the 454Ch no-op (removing it returns PLAYER_UNSUPPORTED in the alt-mode hurt
cases); BuildDecorList's pair count (the composed bonus-exit case fails without the fix);
the child handlers, boss parts and fight, level 5 and level 9 scripts, contacts, terrain,
camera, cave scrolling, landing, side-tile spikes, game over and ending paths (about 60
mutants across the objects, effects, player and level suites).

## 13. Native integration results

docs/smoke-test.md: the 50 s smoke test reaches level 1 with 72.97 Hz timer interrupts,
24.30 frames/s, 3.00 ticks/frame, 50.13 Sound Blaster blocks/s, VGA bus time 8.9 ms of the
41.2 ms frame; the four transition scenarios (warp into bonus room 0Ah, ending, game over,
CASTLE) reach their next gameplay natively.

## 14. Known differences from the original

- Interrupts arrive only at service points (busy-waits, frame waits, 3DAh reads, loads),
  not between arbitrary instructions: a timing the hardware can produce; loops that
  depend on an interrupt inside a two-instruction window (the F2 spin) never see it.
- Level codes are those of a machine whose BIOS ROM fold is zero (CPU class 3).
- The ESC quit exits the host process instead of running the original's memory wipe and
  DOS exit.
- Presentation is sampled once per CRT frame at display start (no mid-frame tearing).
- Load times and VGA bus time are a reference machine's (adjustable: --fast-load,
  --fast-vga), not any particular historical PC's.
