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

## Native consequence

Simulation runs in logical time on native/machine.c (docs/native.md, "Timing boundary"): PIT
ticks at the programmed divisor, frames every 3 ticks through WaitSoundTicks, audio blocks on
their own 119 us x 168 clock, CRT edges from the CRTC timing registers; logical time advances
only by waits, busy-wait turns, video bus time and period load time. The host maps wall time
to a logical bound and never drives the game from the display or the audio device. Interrupts
arrive at the game's service points (busy-waits, frame waits, 3DAh reads, loads): one of the
timings real hardware produces, not an equivalence to every interleaving.
