# Native integration smoke test

```powershell
python tools/hostbuild.py
build\host\pre2.exe --assets assets --smoke 45
```

The only run of the complete program. It is an integration and pacing check, not a
behavioural proof: behaviour is established routine by routine by tools/difftest.py.

Procedure: the native program starts (our own assembled image, no PRE2.EXE, no emulator,
no Python); a scripted key source presses RETURN for 0.25 s every 1.2 s until the first
main-loop frame, then holds RIGHT; after the wall-clock limit the host prints its counters,
writes the last frame (smoke_last.bmp) and exits with status 3. A watchdog thread samples
the game thread if it stops reaching service points (a wait that never polls).

## Result (2026-10-12, 45 s run)

| Check | Measured | Expected (docs/timing.md) |
|---|---|---|
| start-up to first gameplay frame | 17.15 s (late-year credits, TITUS, PRESENT, front end, map scroll-in) | — |
| timer interrupts in gameplay | 72.85 Hz | 72.83 Hz (PIT divisor 4000h) |
| simulation frames | 24.27 frames/s | 24.28 frames/s |
| ticks per frame | 3.00 | 3 (WaitSoundTicks) |
| Sound Blaster block IRQs in gameplay | 50.05 /s | 50.02 /s (8403 Hz / 168 bytes) |
| PCM output in gameplay | 8408 bytes/s | 8403 bytes/s |
| frames presented | 3025 in 45 s (display rate, independent of simulation) | — |
| input | RIGHT held: PlayerX 002Ah -> 0236h on level 1 | the player walks right |
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

Limits: one level, a few seconds of input; no claim of exhaustive gameplay parity. Level
codes depend on the BIOS ROM fold (the native host's F000:FFF0 area is zero) and CPU class
3 (docs/next-work.md).
