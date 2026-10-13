# Native integration smoke test

```powershell
python tools/hostbuild.py
build\host\pre2.exe --assets assets --smoke 50 [--shots DIR]
```

The only run of the complete program. It is an integration and pacing check, not a
behavioural proof: behaviour is established routine by routine by tools/difftest.py.

Procedure: the native program starts (our own assembled image, no PRE2.EXE, no emulator,
no Python); a scripted key source taps RETURN (0.1 s, a quick human press) every 1.2 s
until the first main-loop frame, keeps its hands off while the world map scrolls in, then
holds RIGHT; after the wall-clock limit the host prints its counters,
writes the last frame (smoke_last.bmp) and exits with status 3. A watchdog thread samples
the game thread if it stops reaching service points (a wait that never polls).

## Result (2026-10-13, 50 s run)

| Check | Measured | Expected (docs/timing.md) |
|---|---|---|
| start-up to first gameplay frame | 33.64 s (late-year credits, TITUS, PRESENT, front end, MODE, map scroll-in; with period load and VGA bus time) | — |
| timer interrupts in gameplay | 72.96 Hz | 72.83 Hz (PIT divisor 4000h) |
| simulation frames | 24.32 frames/s | 24.28 frames/s |
| ticks per frame | 3.00 | 3 (WaitSoundTicks) |
| Sound Blaster block IRQs in gameplay | 50.11 /s | 50.02 /s (8403 Hz / 168 bytes) |
| PCM output in gameplay | 8418 bytes/s | 8403 bytes/s |
| VGA bus time in gameplay | 17615 memory + 106 port accesses per frame: 8.9 ms of the 41.2 ms frame | fits the frame |
| frames presented | 3478 in 50 s (one per 70.1 Hz CRT frame, at its display start) | 70.1 Hz |
| MODE screen | one frame loop pass per CRT frame: MapScrollX -4 every 14.27 ms | once per retrace |
| map scroll-in | from black, start address and pel panning consistent in every frame | smooth 1 px/frame |
| input | RIGHT held: PlayerX -> 0236h on level 1 | the player walks right |
| frame | level 1 with player, scenery and HUD (lives 2, score, energy) | — |

Problems the smoke test found, each fixed at its lowest level:
- WaitForButtonPressRelease busy-polled the input state without a service point (the
  original relies on the keyboard interrupt arriving during the loop): it now calls
  platform_poll(). (Behaviour in the differential tests is unchanged: poll is a no-op there.)
- StartSbBlock had been left as a boundary stub that also cut the caller's control flow
  short (ActivateResourceSlot did not set SoundState bit 2; the F3 key path skipped its key
  table update). It is now translated (3 new cases) and the callers follow the original.
- The host's SB interrupt model raised IRQs while the queue was low even without a
  completed block; it now raises exactly one IRQ per started block.

Presentation problems (reported 2026-10-12: TITUS and intro glitches, mode select not
shown, map not scrolling in from black, gameplay shifted horizontally), each fixed at its
lowest level:
- The VGA BIOS mode set (INT 10h AH=00h) only recorded the mode number. It is now modelled
  (c/vga_bios.c, the same function on both sides of the differential tests): the full
  register set of modes 03h, 0Dh, 12h and 13h (misc, sequencer, CRTC, graphics, attribute),
  the default DAC and the memory clear unless AL bit 7 is set. The game relies on it: the
  map screens narrow the display (CRTC 01h = 26h), wrap it at 8 KiB (CRTC 17h = E2h) and
  pan it (attribute 13h), and leave the next mode set to restore them; without it the
  level kept the map screen's geometry (the horizontal shift) and VRAM kept old pictures.
- The presenter only knew mode 0Dh. host/present.c now scans out from register state
  (display end, character height, scan doubling, offset, byte/word/doubleword mode,
  CRTC 17h address substitution, line compare, panning, 256-colour chain 4, palette path,
  screen-off and PAS blanking), so mode 13h (TITUS, PRESENT, front end) and the narrowed,
  wrapped, panned map screens show as on the VGA.
- Loads were instantaneous. The front end starts the MODE screen while RETURN is down, and
  the MODE screen and the map scroll-in both end on their first frame if RETURN is still
  held (KeyDownTable[1Ch]); on the original machine (no EMS/XMS: each resource is read from
  disk and decoded on every use) the load before the first frame outlasts a key press. The
  host now charges each DOS open and read a period machine's time, servicing interrupts
  meanwhile (host_io in host/platform_host.c; --fast-load turns it off). The game code is
  unchanged: holding RETURN through the load still skips both screens, as on the original.
- The map scroll-in jumped by 8 pixels. The game writes the new start address before the
  retrace and the pel panning during it; the CRTC latches the start address at the start
  of vertical retrace, so each frame shows a consistent pair. The host presented at an
  arbitrary moment. It now runs a CRT timeline from the CRTC timing registers (31.78 us
  lines, 449 per frame): 3DAh bits 3 and 0 from the scan position, the start address
  latched at the retrace start, and each frame scanned out at its display start. The
  timeline advances before every VGA register access (vga_access_hook), so writes take
  effect in time order.
- The MODE screen's background moved far too fast. Its frame loop waits only for the
  retrace bit to be set (WaitVerticalRetraceStart), and it sets CRTC 11h = 24h: an 8-line
  (254 us) retrace. On the original each pass (two strings, a column and a row copy:
  thousands of VGA accesses over the ISA bus) outlasted the retrace, so it ran once per
  frame; on the host a pass took microseconds and ran dozens of times per retrace. The
  host now charges VGA accesses ISA bus time (0.5 us per memory access, 1 us per port
  access; counted by the model, paid by spinning; --fast-vga turns it off).

## Transition scenarios (`--scenario NAME`, 2026-10-13)

`build\host\pre2.exe --assets assets --smoke 75 --scenario warp|ending|gameover|castle`: 2 s
into level 1 the harness writes the state that triggers the transition, then follows the
program through its screens with RETURN taps and prints each top-level step and level
change. All four reach their next gameplay natively:

| Scenario | State written | Observed |
|---|---|---|
| warp | CurrentLevel 3, LevelEndMode 2 | warp taken, bonus room 0Ah running (player at its start X 35h; frame checked) |
| ending | EndSignal FFh | THEEND and the button wait in the frame loop, front end, map, a new game on level 0 |
| gameover | EndSignal 1 | death bounce, game-over screen, front end, map, level 0 |
| castle | CurrentLevel 7, LevelEndMode 1, beginner | exit sequence into level 8, CASTLE picture and button wait, front end, map, level 0 |

(The gameplay rates printed in scenario runs include those screens; the cadence figures
are the plain smoke test's.)

Limits: one level, a few seconds of input; no claim of exhaustive gameplay parity. Level
codes depend on the BIOS ROM fold (the native host's F000:FFF0 area is zero) and CPU class
3 (docs/next-work.md).
