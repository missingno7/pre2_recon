# Timing model (from the ASM; static)

PIT input clock 1193182 Hz. All rates below follow from the divisors the program writes.

| Configuration | INT 08h handler | PIT ch0 divisor | Handler rate | TimerTickCount rate |
|---|---|---|---|---|
| Sound Blaster or no sound | SpeakerTimerIsr (cs:075C) | 4000h (startup, ProgramEntry) | 72.83 Hz | 72.83 Hz |
| AdLib sample output | SampleTimerIsr (cs:06F6, installed by InitAdlibSampler) | 008Eh | 8402.7 Hz | 65.65 Hz (every 128th call) |

- Both handlers end in the tick path (loc_07AD): `TimerTickCount++` (cs:1D67) and, every
  4th tick, the BIOS tick body (loc_17C0: the 0040:006C day counter, floppy motor timeout),
  so the BIOS clock keeps 18.2 Hz in the default configuration (72.83 / 4).
- While a DOS call is active (DosCallActive, ds:2877 != 0) SampleTimerIsr reprograms the PIT
  to 4000h and takes the tick path on every interrupt; it restores 8Eh afterwards.
- A gameplay frame ends in PresentFrame (FlipDisplayPage + WaitSoundTicks). WaitSoundTicks
  busy-waits until |TimerTickCount - word_27EE| >= 3, then stores TimerTickCount in
  word_27EE; byte_6BE7 = 1 when the advance was exactly 3 (the frame kept up), 0 when the
  frame was late. So the simulation runs at one frame per 3 ticks: **24.28 frames/s** with
  the Sound Blaster (or no sound), **21.88 frames/s** in the AdLib sample configuration.
  A late frame is not caught up: the next frame starts from the current tick.
- Sound Blaster output: 8403 Hz (time constant 89h), 168-byte DMA blocks, one SbIrqHandler
  per block = 50.02 Hz. The tracker tick (TrackerTick) runs in that handler, so music tempo
  is tied to the 50 Hz block rate, independent of the frame rate.
- word_27F0 is incremented in the BIOS tick body (18.2 Hz) and used by screens that wait
  for a duration (credits, game over).

## Native host consequence

Simulation frames are driven by an emulated TimerTickCount at 72.83 Hz (the SB
configuration, the one the port targets), not by display refresh. Presentation shows the
page the game flipped to (CRTC start address) whenever it changes; audio consumes the
168-byte blocks the translated mixer produces at 50.02 blocks/s. The original's interrupt
work (timer tick, keyboard, SB block) is delivered at the game's own polling points
(busy waits on TimerTickCount, retrace, keys). Interrupts that arrive exactly there are one
of the timings real hardware produces, so this is a faithful choice among original behaviours,
not an equivalence to every interleaving. Still to verify per handler: that no main-code
read-modify-write races with an interrupt in a way the original depends on (e.g. the in-DOS
flag protocol around DOS calls).
