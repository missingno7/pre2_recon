/* Interrupt bodies of the original (oracle: src/MODULE1.ASM), as C functions the native host
 * calls when the interrupt is due. Each one performs the handler's state writes and port
 * traffic, including the end-of-interrupt write (out 20h, 20h) that the original issues
 * before its IRET. The IRET itself and the register save/restore belong to the host's
 * interrupt entry and are not part of these functions.
 *
 *   loc_07AD        tick path: TimerTickCount, the 4-tick divider, then loc_17C0 or EOI.
 *   loc_17C0        BIOS tick body: word_27F0/27F2 count, 40:40h motor timeout, 40:6Ch
 *                   day counter with the midnight wrap at 1800B0h. While DOS is busy
 *                   (the in-DOS flag, see below) it chains to the previous INT 08h vector.
 *   SpeakerTimerIsr PC speaker sequence (PIT ch2, port 42h), then the tick path. Installed
 *                   at the start; replaced by SampleTimerIsr or the sound device.
 *   SampleTimerIsr  AdLib sample output through port 389h (PIT ch0 = 142), then the tick path
 *                   every 128th call.
 *   SbIrqHandler    SB 8-bit DMA block: acknowledge, buffer swap, DMA channel 1, DSP 14h
 *                   block command, TrackerTick (unless OptionBits 40h), the block mix
 *                   (SbMixBlock), and EOI.
 *
 * Platform service:
 *   platform_chain_int08(seg, off)  the original's `jmp dword ptr cs:Int08SwapSlot`: the
 *                   previous INT 08h vector runs with the interrupt frame. The host calls
 *                   that vector; the test model only counts the chains.
 */
#ifndef INTERRUPTS_H
#define INTERRUPTS_H
#include <stdint.h>

void platform_chain_int08(uint16_t seg, uint16_t off);

void loc_07AD(void);
void loc_17C0(void);
void SpeakerTimerIsr(void);
void SampleTimerIsr(void);
void SbIrqHandler(uint16_t sp);
#endif
