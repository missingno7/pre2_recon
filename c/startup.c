/* Start-up services (oracle: src/MODULE1.ASM ProbeCpuClass cs:1C0Ch, InitSoundDevices cs:0802h,
 * InitAdlibSampler cs:0899h, DetectSoundBlaster steps 4..8 cs:1E74h..1F6Fh, DspWriteChecked
 * cs:1CB0h, DspWriteWait cs:1C97h, OplWriteRegister cs:1CEBh, InitResourceMemory cs:08F0h,
 * LoadRawFile cs:0F7Fh, LinearDmaAddress cs:2041h).
 *
 * Boundary of the sound set-up. The probes (DetectSoundBlaster steps 1..3 and DetectAdLib)
 * are platform_sound_detect: they write only the probe's own port traffic and the DMA test,
 * and report the result. Everything the original does after the probe is translated here:
 * the SbBasePort/SbReadPort/SbWritePort/SbAckPort words and IrqProbeCounts that the probe
 * left in memory, SbIrqLine, the PIC mask, VolumeTable at DATA:12C1h, the INT 08h+irq vector
 * (IVT), the DMA block addresses, the module segment, the in-place 6-bit rescale of the SFX
 * bank, and the DSP start commands (D1h, 40h, 89h); the AdLib sampler (InitAdlibSampler) and
 * the PC speaker fallback. The probes' own port traffic is not part of this translation.
 *
 * Quirks kept: the PIC mask saved in PicMaskSaved is read before anything else; the
 * SfxBankParagraphs value is NextFreeSegment - segment (neg of seg - NFS); the SFX rescale
 * with zero paragraphs loops 65536 times (LOOP with CX = 0); LinearDmaAddress recomputes with
 * DATA:1219h when a block would cross a 64 KiB page; the DSP write poll ends on CX = 0 even
 * when the last poll saw bit 7 set (jcxz then reports a timeout). The EMS and XMS resource
 * placement is not modelled: when those drivers answer, the routine returns
 * STARTUP_UNSUPPORTED instead of placing the resources.
 *
 * The OPL and DSP busy-waits of the original are timing only and are not modelled: the
 * state they wait on (the OPL status at 388h) is polled once per loop, which is observed.
 *
 * Platform test model (the recording versions of the platform services): the EXPORT
 * functions pre2_platform_* set what the platform reports; the defaults are "CPU 386, no
 * Sound Blaster, no AdLib, no EMS, no XMS, drive A, SS = the harness stack segment 1010h,
 * RTC 1980 by the image default". */
#include "pre2.h"
#include "ports.h"
#include "dos_files.h"
#include "sqz.h"
#include "system.h"
#include "startup.h"
#include <string.h>

#define DMA_SCAN_LIMIT 0xFF57u          /* LinearDmaAddress: block crosses a 64 KiB page above this */

/* ---- platform test model ------------------------------------------------------------ */

static struct {
    uint16_t ax, cx, dx; int carry;
    int cls;
    SoundProbe sound;
    uint16_t ss;
    uint8_t ems_ah, drive, xms_al;
} plat = { 0, 0, 0, 0, 3, {0}, LOAD_SEG, 0x84, 0, 0 };

EXPORT void pre2_platform_rtc(uint16_t ax, uint16_t cx, uint16_t dx, int carry)
{
    plat.ax = ax; plat.cx = cx; plat.dx = dx; plat.carry = carry;
}

EXPORT void pre2_platform_cpu(int cls) { plat.cls = cls; }

EXPORT void pre2_platform_sound(int sb, uint16_t base, int irq, int adlib)
{
    memset(&plat.sound, 0, sizeof plat.sound);
    plat.sound.sb_found = sb != 0;
    plat.sound.sb_base = base;
    plat.sound.sb_irq = (uint8_t)irq;
    if (sb) plat.sound.irq_counts[irq & 7] = 1;   /* the counting handler of that IRQ ran once */
    plat.sound.adlib_found = adlib != 0;
}

EXPORT void pre2_platform_misc(uint16_t ss, uint8_t ems_ah, uint8_t drive, uint8_t xms_al)
{
    plat.ss = ss; plat.ems_ah = ems_ah; plat.drive = drive; plat.xms_al = xms_al;
}

void platform_rtc_read_date(uint16_t *ax, uint16_t *cx, uint16_t *dx, int *carry)
{
    *ax = plat.ax; *cx = plat.cx; *dx = plat.dx; *carry = plat.carry;
}

int platform_cpu_class(void) { return plat.cls; }

void platform_sound_detect(SoundProbe *out)
{
    *out = plat.sound;
}

uint16_t platform_stack_segment(void) { return plat.ss; }
uint8_t  platform_ems_int67_40h(void) { return plat.ems_ah; }
uint8_t  platform_dos_drive(void) { return plat.drive; }
uint8_t  platform_xms_installed(void) { return plat.xms_al; }

/* ---- ports and devices ------------------------------------------------------------------ */

/* DspWriteWait (cs:1C97h): no timeout, as the original (used after DetectSoundBlaster). */
void DspWriteWait(uint8_t v)
{
    uint16_t dx = CW(CS_SbWritePort);
    while (port_in8(dx) & 0x80) { }
    port_out8(dx, v);
}

/* DspWriteChecked (cs:1CB0h): at most 10 rounds of up to 65535 polls for bit 7 clear.
 * Returns 0 when the byte was written, 1 on timeout (the original then prints a message
 * and waits, a platform boundary). */
static int DspWriteChecked(uint8_t v)
{
    uint16_t dx = CW(CS_SbWritePort);
    for (int round = 0; round < 10; round++) {
        uint16_t cx = 0xFFFFu;
        uint8_t al;
        do {                                  /* in al, dx; test al, 80h; loopne */
            al = port_in8(dx);
            cx--;
        } while (cx != 0 && (al & 0x80));
        if (cx != 0) { port_out8(dx, v); return 0; }   /* jcxz not taken */
    }
    return 1;
}

/* OplWriteRegister (cs:1CEBh): address port 388h, data port 389h. AL = register, AH = value. */
static void OplWrite(uint16_t ax)
{
    port_out8(0x388, (uint8_t)ax);
    port_out8(0x389, (uint8_t)(ax >> 8));
}

/* ---- AdLib sampler (cs:0899h) ----------------------------------------------------------- */

static void InitAdlibSampler(void)
{
    static const uint16_t setup[] = {
        0x2001, 0x00BD, 0x00B0, 0x2120, 0xF060, 0xF080, 0x01C0, 0x00E0,
        0x3F43, 0x6004, 0xF302, 0x8FA0, 0x2EB0, 0x2304 };
    for (unsigned i = 0; i < sizeof setup / sizeof setup[0]; i++) OplWrite(setup[i]);
    while (port_in8(0x388) & 0xF0) { }          /* wait for the OPL status to clear */
    OplWrite(0x00A0);
    OplWrite(0x20B0);
    OplWrite(0x3F40);
    port_out8(0x40, 0x8E);                       /* PIT ch0 = 142 (about 8.4 kHz) */
    port_out8(0x40, 0x00);
    wr16(0x0000, 0x0020, 0x06F6);                /* IVT 08h = SampleTimerIsr (cs:06F6h) */
    wr16(0x0000, 0x0022, CODE_SEG);
}

/* ---- sound set-up after the probe ---------------------------------------------------------- */

/* VolumeTable[v*64 + s] = (v*s) >> 6 for v = 0..64, s = 0..63 (loc_1E90). */
static void BuildVolumeTable(void)
{
    uint16_t di = 0x12C1u;
    for (unsigned v = 0; v <= 0x40; v++)
        for (unsigned s = 0; s < 0x40; s++)
            wr8(DATA_SEG, di++, (uint8_t)((v * s) >> 6));
}

/* LinearDmaAddress (cs:2041h): DATA-relative segment:offset to the DMA page (linear bits
 * 16..19) and the offset (bits 0..15). When the 168-byte block would cross a 64 KiB page the
 * address is recomputed from DATA:1219h, and the mixer buffers then disagree with DMA (quirk
 * kept, dormant unless the buffers sit at the top of a page). */
static void LinearDmaAddress(uint16_t seg, uint16_t off, uint8_t *page, uint16_t *lo)
{
    for (;;) {
        uint32_t sum = (uint32_t)(uint16_t)(seg << 4) + off;
        uint16_t dx = (uint16_t)sum;
        uint8_t pg = (uint8_t)((seg >> 12) + (sum > 0xFFFFu ? 1 : 0));
        if (dx <= DMA_SCAN_LIMIT) { *page = pg; *lo = dx; return; }
        seg = DATA_SEG;
        off = 0x1219u;
    }
}

/* DetectSoundBlaster steps 4..8, after a successful probe. Returns STARTUP_UNSUPPORTED on a
 * DSP timeout or a memory shortage (both print a message in the original). */
static int SbSetup(const SoundProbe *p)
{
    uint8_t irq = p->sb_irq;
    SET_DB(DS_SbIrqLine, irq);
    port_out8(0x21, (uint8_t)(DB(DS_PicMaskSaved) & (uint8_t)~(1u << irq)));
    BuildVolumeTable();

    uint16_t slot = (uint16_t)((8u + irq) * 4u);  /* IVT entry of INT 08h+irq */
    uint16_t old_off = rd16(0x0000, slot), old_seg = rd16(0x0000, (uint16_t)(slot + 2));
    wr16(0x0000, slot, CS_SbIrqHandler);
    wr16(0x0000, (uint16_t)(slot + 2), CODE_SEG);
    SET_DW(DS_SbSavedIrqOffset, old_off);
    SET_DW(DS_SbSavedIrqSegment, old_seg);

    uint8_t page; uint16_t lo;
    LinearDmaAddress(DATA_SEG, 0x10C9u, &page, &lo);
    SET_DB(DS_DmaPageA, page);
    SET_DW(DS_DmaOffsetA, lo);
    LinearDmaAddress(DATA_SEG, 0x1171u, &page, &lo);
    SET_DB(DS_DmaPageB, page);
    SET_DW(DS_DmaOffsetB, lo);

    if (!(CW(CS_OptionBits) & 0x40u)) {          /* module segment, when memory allows */
        uint16_t room = (uint16_t)(CW(CS_ConventionalTopSeg) - platform_stack_segment());
        if (room < 0x7D00u) return STARTUP_UNSUPPORTED;   /* "not enough memory" message */
        SET_DW(DS_ModuleSegment, DW(DS_NextFreeSegment));
        SET_DW(DS_NextFreeSegment, (uint16_t)(DW(DS_NextFreeSegment) + 0x105Cu));
    }

    /* SFX bank: 8-bit unsigned samples to 6-bit signed-biased: (b + 80h) >> 2. */
    uint16_t seg = DW(DS_SfxBankSegment);
    uint16_t count = (uint16_t)(DW(DS_SfxBankParagraphs) << 4);
    uint32_t n = count ? count : 0x10000u;       /* loop with CX = 0 runs 65536 times */
    uint16_t di = 0;
    for (uint32_t i = 0; i < n; i++, di++) {
        uint8_t b = (uint8_t)(rd8(seg, di) + 0x80u);
        wr8(seg, di, (uint8_t)(b >> 2));
    }

    SET_CB(CS_SoundState, CB(CS_SoundState) | 1u);
    if (DspWriteChecked(0xD1) || DspWriteChecked(0x40) || DspWriteChecked(0x89))
        return STARTUP_UNSUPPORTED;
    return STARTUP_OK;
}

/* ---- ProbeCpuClass (cs:1C0Ch) ---------------------------------------------------------- */

/* The original hooks INT 06h to CpuProbeInt06Handler (cs:1C4Bh) while it executes a 386
 * instruction; the handler sets CpuClass = 2 and the shift-count test sets 0 or 3. The
 * IVT words are saved and restored, so the net effect is CpuClass alone. The platform
 * reports the class; the vector is written for the duration of the probe as in the original
 * (no code runs through it here). */
void ProbeCpuClass(void)
{
    uint16_t saved_off = rd16(0x0000, 0x0018), saved_seg = rd16(0x0000, 0x001A);
    wr16(0x0000, 0x0018, 0x1C4B);
    wr16(0x0000, 0x001A, CODE_SEG);
    SET_CB(CS_CpuClass, (uint8_t)platform_cpu_class());
    wr16(0x0000, 0x001A, saved_seg);
    wr16(0x0000, 0x0018, saved_off);
}

/* ---- InitSoundDevices (cs:0802h) --------------------------------------------------------- */

int InitSoundDevices(void)
{
    SET_DB(DS_PicMaskSaved, port_in8(0x21));
    if (CB(CS_SoundEnabledFlag) == 0) return STARTUP_OK;      /* /M */

    uint16_t seg;
    int e = LoadSqzFile(0x0859u, &seg);                       /* SAMPLE.SQZ */
    if (e) return e;
    SET_DW(DS_SfxBankSegment, seg);
    SET_DW(DS_SfxBankParagraphs, (uint16_t)(DW(DS_NextFreeSegment) - seg));

    uint16_t acc = 0;                                         /* 11 (start, length) pairs */
    for (uint16_t k = 0; k < 0x0B; k++) {
        uint16_t si = (uint16_t)(0x1009u + 4 * k);
        uint16_t len = DW((uint16_t)(si + 2));
        SET_DW(si, acc);
        acc = (uint16_t)(acc + len);
    }

    SoundProbe p;
    platform_sound_detect(&p);
    if (p.sb_found) {
        /* What the probe stored: the DSP port words and the IRQ counters. */
        SET_CW(CS_SbBasePort, p.sb_base);
        SET_CW(CS_SbReadPort, (uint16_t)(p.sb_base + 0x0A));
        SET_CW(CS_SbWritePort, (uint16_t)(p.sb_base + 0x0C));
        SET_CW(CS_SbAckPort, (uint16_t)(p.sb_base + 0x0E));
        for (unsigned k = 0; k < 8; k++) SET_DB(DS_IrqProbeCounts + k, p.irq_counts[k]);
        e = SbSetup(&p);
        if (e) return e;
        SET_CB(CS_SbDetected, 1);
        return STARTUP_OK;
    }
    if (p.adlib_found && CB(CS_CpuClass) >= 2) {
        InitAdlibSampler();
        SET_CB(CS_AdLibEnabled, 1);
        SET_CB(CS_SoundState, 1);
        return STARTUP_OK;
    }
    /* PC speaker: the sample file is discarded and its memory given back. */
    SET_DW(DS_NextFreeSegment, DW(DS_SfxBankSegment));
    port_out8(0x43, 0xB6);
    port_out8(0x61, (uint8_t)(port_in8(0x61) | 3));
    return STARTUP_OK;
}

/* ---- InitResourceMemory (cs:08F0h) ----------------------------------------------------------- */

/* EMS: the INT 67h vector's device name is compared with DATA:0B49h (4 words at ES:000Ah).
 * XMS: the current drive must be A or B and INT 2Fh AX=4300h must report a driver. */
static int EmsNamePresent(void)
{
    uint16_t es = rd16(0x0000, 0x019E);
    for (uint16_t i = 0; i < 4; i++)
        if (rd16(es, (uint16_t)(0x000A + 2 * i)) != DW((uint16_t)(0x0B49 + 2 * i))) return 0;
    return 1;
}

int InitResourceMemory(void)
{
    if (!(CW(CS_OptionBits) & 0x10u)) {                       /* /L: every resource from disk */
        for (uint16_t i = 0; i < 0x12; i++)
            SET_DB((uint16_t)(DS_ResourceFlags + i), DB((uint16_t)(DS_ResourceFlags + i)) & 0xFDu);
        return STARTUP_OK;
    }
    if ((CW(CS_OptionBits) & 0x04u) && EmsNamePresent()) {
        if (platform_ems_int67_40h() == 0) return STARTUP_UNSUPPORTED;   /* EMS: not modelled */
    }
    if (CW(CS_OptionBits) & 0x08u) {
        if (platform_dos_drive() <= 1 && platform_xms_installed() == 0x80)
            return STARTUP_UNSUPPORTED;                                    /* XMS: not modelled */
    }

    for (uint16_t i = 0; i < 0x12; i++) {
        uint16_t si = DB((uint16_t)(DS_ResourceSlotList + i));
        uint8_t flags = DB((uint16_t)(DS_ResourceFlags + si));
        if (CB(CS_SbDetected) == 1) {
            if (!(flags & 0x08u) && (CW(CS_OptionBits) & 0x40u)) continue;
        } else if (!(flags & 0x08u)) {
            continue;
        }
        uint16_t size = DB((uint16_t)(DS_ResourceSizeKb + si));
        if (DW(DS_EmsFreeKb) >= size && DW(DS_EmsHandle) != 0x55AAu)
            return STARTUP_UNSUPPORTED;                               /* LoadResourceToEms */
        if (DW(DS_XmsFreeKb) >= size && DW(DS_XmsHandle) != 0x55AAu && !(flags & 0x10u))
            return STARTUP_UNSUPPORTED;                               /* LoadResourceToXms */
        if (!(flags & 0x02u)) continue;                               /* not resident */
        if (CW(CS_OptionBits) & 0x20u) {                              /* LoadResourceFile */
            uint16_t seg;
            int st = LoadRawFile(ResourceNameAt(si), &seg);
            if (st) return st;
            SET_DW((uint16_t)(DS_ResourceAddressTable + 4 * si), seg);
        } else {
            SET_DB((uint16_t)(DS_ResourceFlags + si), flags & 0xFDu);
        }
    }
    return STARTUP_OK;
}

/* ---- LoadRawFile (cs:0F7Fh) ----------------------------------------------------------------- */

/* Reads the whole file named at DATA:name into the heap at NextFreeSegment (one read of up to
 * FFFEh bytes), closes it and advances NextFreeSegment by bytes/16 + 1. *seg receives the
 * segment of the data (the old NextFreeSegment). RawLoadInProgress is 1 while it runs. Open
 * and read errors print a message and wait in the original; here they return
 * STARTUP_UNSUPPORTED with RawLoadInProgress still 1, as the original leaves it. */
int LoadRawFile(uint16_t name, uint16_t *seg)
{
    SET_DB(DS_RawLoadInProgress, 1);
    uint16_t handle;
    if (dos_open(DATA_SEG, name, &handle)) return STARTUP_UNSUPPORTED;   /* ReportOpenError */
    uint16_t buf = DW(DS_NextFreeSegment);
    uint16_t n;
    if (dos_read(handle, buf, 0, 0xFFFEu, &n)) return STARTUP_UNSUPPORTED; /* ReportReadError */
    uint16_t paras = (uint16_t)((n >> 4) + 1);
    SET_DW(DS_NextFreeSegment, (uint16_t)(DW(DS_NextFreeSegment) + paras));
    uint16_t ax;
    (void)dos_close(handle, &ax);          /* the original ignores the close result */
    SET_DB(DS_RawLoadInProgress, 0);
    *seg = buf;
    return STARTUP_OK;
}

/* ---- difftest adapters (tests/startup.py) ----------------------------------------------- */

EXPORT int c_ProbeCpuClass(Regs *r)
{
    ProbeCpuClass();
    r->dx = 0x02C5u;                           /* left over by the probe */
    r->ds = DATA_SEG;
    return 0;
}

EXPORT int c_InitSoundDevices(Regs *r)
{
    int e = InitSoundDevices();
    r->ds = DATA_SEG;
    return e;
}

EXPORT int c_InitResourceMemory(Regs *r)
{
    int e = InitResourceMemory();
    r->ds = DATA_SEG;
    return e;
}

EXPORT int c_LoadRawFile(Regs *r)
{
    uint16_t seg = 0;
    int e = LoadRawFile(r->dx, &seg);
    r->ax = seg;
    r->ds = DATA_SEG;
    return e;
}
