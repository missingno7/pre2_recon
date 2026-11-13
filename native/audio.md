# Faithful audio boundary

`audio_assets.c` owns pristine SQZ-decoded SAMPLE and all twelve TRK assets, and
builds destructive module playback images. `audio_tracker.c` owns sequencing;
`audio_mix.c` owns integer mixing; `audio.c` owns command application and the
zero/stale double-buffer pipeline. `machine.c` supplies logical service-point
ordering and consumes timestamped blocks. The engine never accesses `pre2_mem`.
The immutable linked ASM image supplies adjacent character/protection and IRQ
lookup bytes exposed by unchecked table accesses; it is not live game memory.

Commands are atomic at the translated origins, the specified interleaving of the
original unprotected SFX cursor/count stores. A block consumes the latest SFX
command at mix time, with no deduplication. PCM remains U8 mono, silence 00h,
8403 Hz reported rate, and an exact 119 us sample clock (19992 us per block).
F3 and load readiness stop rearming after the pending completion; restarts submit
the selected stale play buffer immediately, before ticking or mixing.

Two additional inputs were required by audit sections 2, 3, 5 and 6:

* `AUDIO_OUTPUT_READY` represents the sound-update tail independently of song
  rebuild. In particular `/B` never rebuilds a module but must set readiness.
* `SONG_END` carries sixteen bytes after the final decoded sample, captured at
  actual activation. Inclusive limits expose stale allocation padding, which
  cannot be reconstructed from the asset. `DEVICE` carries SAMPLE's converted
  allocation padding. Neither requires the engine to inspect game memory.

The private DATA arena preserves wrong-index instrument stores, unsigned XLAT,
wrapped sums, loop extra iterations, accumulated-output fades and inter-buffer
overruns. Its module backing covers all byte-sized pattern indices and a wrapped
row offset. Beyond the owned loader window that backing starts at zero, matching
the constructed boot arena. BOULA contains B80: order 128 reads the adjacent IRQ
vector table's FFh byte as pattern 255. Tests preserve that unchecked path rather
than range-clamping the order. Arbitrary corruption or unrelated future writes
outside the owned module allocation are not claimed to be an emulated address
space. The native engine intentionally owns only audio state.

`audio_parity.c` records and compares every PCM block, timestamp and semantic
command in six scripted scenarios, and hashes memory below A0000h at every
main-loop frame. Half-open DATA exclusions (audit section 4) are:

| Range | Private reference IRQ updates |
|---|---|
| 0B5D..0B5E | Completion count; ASM ESC wait is absent from translated quit |
| 0B60..0B64 | Saved SP and mixer channel scratch |
| 0B84..0DC2 | Sequencer, channels and aliased instrument records |
| 1004..1008 | SFX position and remaining length |
| 10B9..10BB | Saved volume row |
| 10C1..12C1 | Buffer selectors, PCM A/B/C and overrun storage |

No other production game consumer reads those mutable audio fields. Resource
flags, allocation state, ModuleSegment, PIT state, SoundState and decoded module/
sample allocations remain included. `audio_songs.c` independently compares all
block publications across two loops of every order list, repeated SFX at varied
block offsets, F3 off/on, changes during SFX, and `/B`. Its transformed asset
images and the entire converted SAMPLE allocation are also compared directly.
