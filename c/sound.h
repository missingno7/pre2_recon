/* Software sound mixer and MOD tracker (oracle: src/MODULE1.ASM). */
#ifndef SOUND_H
#define SOUND_H
#include <stdint.h>

#define MIX_BLOCK_BYTES 0xA8u     /* one 168-byte DMA block (MixBuffer) */

/* Adds channel `ch` (channel offset 0, 2, 4, 6) into MixBuffer. `entry_sp` is the SP the
 * original has at this call; MixChannel stores it in MixSavedSp (see sound.c). */
void MixChannel(uint16_t ch, uint16_t entry_sp);

/* Block-mix body of SbIrqHandler (entered at loc_20CF): SFX copy or silence, then
 * MixChannel for channels 0, 2, 4 and 6 (6 only when no SFX remains). `sp` is the SP at
 * entry. Port I/O (EOI) is not included. */
void SbMixBlock(uint16_t sp);

/* One 50 Hz tracker tick (volume slides, row countdown, row processing). */
void TrackerTick(void);

/* Busy-waits for the 50 Hz tick to advance by 3 or more (moved from render.c). */
void WaitSoundTicks(void);

/* Applies the 4-byte cell at es:si to channel `ch` (channel offset). */
void ProcessChannelCell(uint16_t es, uint16_t si, uint16_t ch);

/* Effect handlers reached through EffectJumpTable. `al` is the effect parameter. */
void EffectVolumeSlide(uint16_t ch, uint8_t al);
void EffectPositionJump(uint8_t al);
void EffectSetVolume(uint16_t ch, uint8_t al);
void EffectPatternBreak(uint8_t al);
void EffectSetSpeed(uint8_t al);
#endif
