/* Interrupt bodies (oracle: src/MODULE1.ASM loc_07AD cs:07ADh, loc_17C0 cs:17C0h,
 * SpeakerTimerIsr cs:075Ch, SampleTimerIsr cs:06F6h, SbIrqHandler cs:204Dh with its mix at
 * loc_20CF). See interrupts.h for the contract.
 *
 * The in-DOS flag (DosCallActive, DATA:2877h) is one byte. The original writes it as
 * cs:DosCallActive (the DOS wrappers, SampleTimerIsr) and reads/writes it as ds:DosCallActive
 * (loc_17C0, the keyboard ISR). The cs: form is assembled with the displacement C667h, the same
 * linear byte as DS:2877h, so both forms are DB(DS_DosCallActive). */
#include "pre2.h"
#include "ports.h"
#include "sound.h"
#include "startup.h"
#include "interrupts.h"

#define DOS_FLAG     DB(DS_DosCallActive)      /* DosCallActive, however the ASM addresses it */

static uint32_t chain_count;                   /* platform test model: chains taken */
EXPORT uint32_t pre2_int08_chains(void) { return chain_count; }
EXPORT void pre2_int08_reset_chains(void) { chain_count = 0; }

void platform_chain_int08(uint16_t seg, uint16_t off)
{
    (void)seg; (void)off;                      /* the test model has no BIOS vector */
    chain_count++;
}

/* loc_17C0: BIOS tick body. The 40h segment is the BIOS data area (linear 400h). */
void loc_17C0(void)
{
    uint32_t t27f0 = (uint32_t)DW(DS_word_27F0) + 1;          /* add word_27F0, 1 */
    SET_DW(DS_word_27F0, (uint16_t)t27f0);
    SET_DW(DS_word_27F2, (uint16_t)(DW(DS_word_27F2) + (t27f0 >> 16)));   /* adc word_27F2, 0 */

    if (DOS_FLAG) {                         /* DOS busy: the BIOS vector does the tick */
        /* dword slot: offset word first, then segment word */
        platform_chain_int08(CW((uint16_t)(CS_Int08SwapSlot + 2)), CW(CS_Int08SwapSlot));
        return;
    }
    uint8_t motor = (uint8_t)(rd8(0x0040, 0x0040) - 1);      /* dec byte ptr 40:40h */
    wr8(0x0040, 0x0040, motor);
    if (motor == 0) {                          /* floppy motor timeout: motors off */
        wr8(0x0040, 0x003F, rd8(0x0040, 0x003F) & 0xF0u);
        port_out8(0x3F2, 0x0C);
    }
    uint32_t ticks = (uint32_t)rd16(0x0040, 0x006C) | (uint32_t)rd16(0x0040, 0x006E) << 16;
    ticks = (uint32_t)(ticks + 1);                             /* inc word [6Ch]; carry into [6Eh] */
    if (ticks == 0x001800B0u) {                /* 24 hours of ticks: midnight wrap */
        ticks = 0;
        wr16(0x0040, 0x0070, 1);               /* midnight flag */
    }
    wr16(0x0040, 0x006C, (uint16_t)ticks);
    wr16(0x0040, 0x006E, (uint16_t)(ticks >> 16));
    port_out8(0x20, 0x20);                     /* EOI */
}

/* loc_07AD: the tick path entered by every INT 08h handler. */
void loc_07AD(void)
{
    SET_CW(CS_TimerTickCount, (uint16_t)(CW(CS_TimerTickCount) + 1));
    uint8_t div = (uint8_t)((CB(CS_cvar_1D6B) + 1) & 3);      /* inc byte; and 3 */
    SET_CB(CS_cvar_1D6B, div);
    if (div != 0) {
        port_out8(0x20, 0x20);                 /* EOI, IRET */
        return;
    }
    loc_17C0();
}

/* SpeakerTimerIsr: plays the PC speaker sequence while no sound device is active. Each
 * record is 10 bytes: +0 divisor, +2 start divisor, +4 count, +6 step, +8 count reload. */
void SpeakerTimerIsr(void)
{
    if (CB(CS_SbDetected) == 0 && CB(CS_AdLibEnabled) == 0) {
        uint16_t si = DW(DS_SpeakerSequencePtr);
        if (si != 0xFFFFu) {
            uint16_t bx = rd16(DATA_SEG, si);
            port_out8(0x42, (uint8_t)bx);
            port_out8(0x42, (uint8_t)(bx >> 8));
            bx = (uint16_t)(bx - rd16(DATA_SEG, (uint16_t)(si + 6)));
            wr16(DATA_SEG, si, bx);
            uint16_t count = (uint16_t)(rd16(DATA_SEG, (uint16_t)(si + 4)) - 1);
            wr16(DATA_SEG, (uint16_t)(si + 4), count);
            if (count == 0) {
                wr16(DATA_SEG, (uint16_t)(si + 4), rd16(DATA_SEG, (uint16_t)(si + 8)));
                wr16(DATA_SEG, si, rd16(DATA_SEG, (uint16_t)(si + 2)));
                SET_DW(DS_SpeakerSequencePtr, 0xFFFFu);
                port_out8(0x42, 0x0A);         /* divisor 000Ah when the record ends */
                port_out8(0x42, 0x00);
            }
        }
    }
    loc_07AD();
}

/* SampleTimerIsr: AdLib sample output (PIT ch0 = 142). While DOS is busy it sets the PIT to
 * 4000h and takes the tick path every time. */
void SampleTimerIsr(void)
{
    if (DOS_FLAG != 0) {                    /* loc_06E9 */
        port_out8(0x40, 0x00);                 /* PIT ch0 = 4000h (18.2 Hz) */
        port_out8(0x40, 0x40);
        loc_07AD();
        return;
    }
    port_out8(0x40, 0x8E);                     /* PIT ch0 = 142 (about 8.4 kHz) */
    port_out8(0x40, 0x00);
    uint16_t n = (uint16_t)((CW(CS_cvar_1D69) + 1) & 0x7F);   /* inc word; and 7Fh */
    SET_CW(CS_cvar_1D69, n);
    if (n == 0) {                              /* every 128th call: tick path */
        loc_07AD();
        return;
    }
    if ((CB(CS_SoundState) & 1) != 0 && DW(DS_SfxBytesLeft) != 0) {
        SET_DW(DS_SfxBytesLeft, (uint16_t)(DW(DS_SfxBytesLeft) - 1));
        uint16_t si = DW(DS_SfxPlayPos);
        SET_DW(DS_SfxPlayPos, (uint16_t)(si + 1));
        uint8_t v = (uint8_t)(rd8(DW(DS_SfxBankSegment), si) ^ 0x80u);
        uint8_t out = rd8(DATA_SEG, (uint16_t)(0x0F05u + (uint8_t)(v >> 1)));   /* xlat */
        port_out8(0x389, out);
    }
    port_out8(0x20, 0x20);                     /* EOI, IRET */
}

/* SbIrqHandler: `sp` is the SP at the mix (loc_20CF) as the original has it, passed to
 * SbMixBlock (MixChannel stores it in MixSavedSp). */
void SbIrqHandler(uint16_t sp)
{
    uint16_t es_saved = pre2_es;               /* push es ... pop es: the handler leaves ES as it found it */
    SET_ES(DATA_SEG);
    (void)port_in8(CW(CS_SbAckPort));          /* acknowledge the block */
    SET_DB(DS_SbIrqCount, (uint8_t)(DB(DS_SbIrqCount) + 1));
    if ((CB(CS_SoundState) & 3) == 3) {        /* device ready and output started */
        uint16_t t = DW(DS_PlayBuffer);        /* swap PlayBuffer <-> MixBuffer */
        SET_DW(DS_PlayBuffer, DW(DS_MixBuffer));
        SET_DW(DS_MixBuffer, t);
        t = DW(DS_PlayDmaRecord);              /* swap the DMA records */
        SET_DW(DS_PlayDmaRecord, DW(DS_MixDmaRecord));
        SET_DW(DS_MixDmaRecord, t);

        uint16_t rec = DW(DS_PlayDmaRecord);   /* record: page byte, offset word */
        port_out8(0x0A, 5);                    /* mask channel 1 */
        port_out8(0x0C, 0);                    /* clear the byte pointer */
        uint16_t off = rd16(DATA_SEG, (uint16_t)(rec + 1));
        port_out8(0x02, (uint8_t)off);
        port_out8(0x02, (uint8_t)(off >> 8));
        port_out8(0x83, rd8(DATA_SEG, rec));   /* page */
        port_out8(0x03, 0xA7);                 /* count 167 = 168 bytes */
        port_out8(0x03, 0x00);
        port_out8(0x0A, 1);                    /* unmask channel 1 */
        DspWriteWait(0x14);                    /* DSP 14h: 8-bit DMA block, length A7h 00h */
        DspWriteWait(0xA7);
        DspWriteWait(0x00);

        if (!(CW(CS_OptionBits) & 0x40u)) TrackerTick();
        SbMixBlock(sp);
    }
    port_out8(0x20, 0x20);                     /* SbIrqExit: EOI */
    pre2_es = es_saved;
}

/* ---- difftest adapters (tests/interrupts.py) -------------------------------------------- */

/* The interrupt frame (IP = 0, CS, FLAGS) is set by the test on the stack; the handlers end
 * with IRET in the original, which the test runs as the stop at cs:0000. */
EXPORT int c_loc_07AD(Regs *r) { (void)r; loc_07AD(); r->ds = DATA_SEG; return 0; }

EXPORT int c_loc_17C0(Regs *r) { (void)r; loc_17C0(); r->ds = DATA_SEG; return 0; }

EXPORT int c_SpeakerTimerIsr(Regs *r) { (void)r; SpeakerTimerIsr(); r->ds = DATA_SEG; return 0; }

EXPORT int c_SampleTimerIsr(Regs *r) { (void)r; SampleTimerIsr(); r->ds = DATA_SEG; return 0; }

/* SP at loc_20CF in the test frame: the entry SP (01FEh) less the nine pushes (18 bytes). */
EXPORT int c_SbIrqHandler(Regs *r) { (void)r; SbIrqHandler(0x01ECu); r->ds = DATA_SEG; return 0; }
