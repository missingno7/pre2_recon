/* System services (oracle: src/MODULE1.ASM; see system.h for the boundaries).
 *
 * Quirks kept: the sample-mode sound effect table index uses the whole DX (DH is not
 * masked); a zero SongLength or PatternCount loops 65536 times (LOOP with CX = 0, DEC
 * with DX = 0); the pattern-cell pass runs PatternCount * 256 cells and rewrites each
 * period word in the loaded module; a pinned resource (ResourceFlags 08h) is loaded but
 * not copied, so the MOD header is read from the stale ModuleSegment; the joystick timing
 * loop runs 65536 times when its count starts at 0; shift counts of the joystick bit
 * shift are masked to 5 bits as the 8086 does; the demo log's run-length counter is a byte
 * and stops at FFh. DS is DATA on entry to every routine (as the callers in R1 leave it).
 */
#include "pre2.h"
#include "ports.h"
#include "vga.h"
#include "sqz.h"
#include "system.h"
#include "render.h"

#define RESOURCE_NAMES   0x09E0u    /* DATA: NUL-terminated resource names, PRES.TRK first */
#define RF(slot)         ((uint16_t)(DS_ResourceFlags + (slot)))
#define JOY_PORT         0x201u
#define KBD_DATA_PORT    0x60u
#define KBD_CTRL_PORT    0x61u
#define PIC_EOI_PORT     0x20u
#define ESC_SCAN         0x01u
#define F3_SCAN          0x3Du

/* 8086 SHR r/m8, CL: the count is masked to 5 bits; counts of 8 or more clear the byte. */
static uint8_t shr8(uint8_t v, uint8_t cl)
{
    unsigned n = cl & 31u;
    return n >= 8 ? 0 : (uint8_t)(v >> n);
}


/* ---- sound effects (cs:0282h) ------------------------------------------------------- */

void StartSoundEffect(uint16_t dx)
{
    if (CB(CS_AdLibEnabled) == 1 || CB(CS_SbDetected) == 1) {
        /* sample mode: BX = DX * 4 from the whole DX (not DL) */
        uint16_t bx = (uint16_t)(dx << 2);
        SET_DW(DS_SfxPlayPos, DW((uint16_t)(bx + DS_word_1009)));
        SET_DW(DS_SfxBytesLeft, DW((uint16_t)(bx + DS_word_100B)));
    } else {
        /* speaker mode: AX = 10 * DL (MUL DL) + 1037h */
        SET_DW(DS_SpeakerSequencePtr, (uint16_t)(0x0Au * (uint8_t)dx + 0x1037u));
    }
}

/* ---- resource table (cs:0492h PageInResource and its tails) -------------------------- */

uint16_t ResourceNameAt(uint16_t index)
{
    uint16_t si = RESOURCE_NAMES;
    if (index != 0) {
        do {                                    /* skip one NUL-terminated name per CX */
            do si = (uint16_t)(si + 1); while (DB(si) != 0);
            index--;
        } while (index != 0);
        si = (uint16_t)(si + 1);
    }
    return si;
}

/* Disk location (code 0): load the SQZ resource into the heap; unless pinned (08h), copy
 * 105Bh paragraphs of it into ModuleSegment. The heap top is restored in both cases. */
static int page_in_disk(uint16_t slot)
{
    uint16_t saved_top = DW(DS_NextFreeSegment);
    uint16_t decoded = 0;
    int st = LoadSqzFile(ResourceNameAt(slot), &decoded);
    if (st) return st;
    if (DB(DS_byte_0B16) == 0) {
        uint16_t dst = DW(DS_ModuleSegment);
        for (uint16_t n = 0x105Bu; n != 0; n--) {   /* rep movsw of 8 words per paragraph */
            for (uint16_t k = 0; k < 8; k++)
                wr16(dst, (uint16_t)(2 * k), rd16(decoded, (uint16_t)(2 * k)));
            dst = (uint16_t)(dst + 1);
            decoded = (uint16_t)(decoded + 1);
        }
    }
    SET_DW(DS_NextFreeSegment, saved_top);
    return SYS_OK;
}

int PageInResource(uint16_t slot)
{
    uint8_t flags = DB(RF(slot));
    SET_DB(DS_byte_0B16, flags & 0x08);          /* pinned bit, 0 or 8 */
    switch ((flags >> 1) & 3) {
    case 0:
        return page_in_disk(slot);
    case 1:                                      /* conventional copy: DecodeLzssStream in memory mode */
        return SYS_UNSUPPORTED;
    case 2:                                      /* EMS pages (INT 67h) */
        return platform_ems_map_pages(DW(DS_EmsHandle), DW((uint16_t)(DS_ResourceAddressTable + 4 * slot)));
    default:                                     /* XMS move (XmsDriverEntry, function 0Bh) */
        return platform_xms_move(DW(DS_XmsHandle), DW((uint16_t)(DS_ResourceAddressTable + 4 * slot)));
    }
}

/* loc_046F: the sound update that ends ActivateResourceSlot. */
static int resource_sound_update(void)
{
    if (CB(CS_SoundState) & 1)
        return platform_sb_start_block();        /* StartSbBlock: boundary */
    SET_CB(CS_SoundState, CB(CS_SoundState) | 2);
    return SYS_OK;
}

/* loc_0300 .. loc_0482: make `slot` active, page it in and rebuild the MOD tables. */
static int activate_slot_body(uint16_t slot)
{
    SET_DB(RF(slot), DB(RF(slot)) | 1);
    for (uint16_t di = 0; di < 0x12; di++)     /* the other slots lose their active bit unless pinned */
        if (di != slot && !(DB(RF(di)) & 8))
            SET_DB(RF(di), DB(RF(di)) & 0xFE);
    SET_CB(CS_SoundState, CB(CS_SoundState) & 0xFD);
    int st = PageInResource(slot);
    if (st) return st;

    /* Instrument headers (MOD sample records, big-endian words, lengths in words). */
    uint16_t mseg = DW(DS_ModuleSegment);
    uint16_t si = 0x14;
    uint16_t count = (rd16(mseg, 0x438) == 0x2E4D && rd16(mseg, 0x43A) == 0x2E4B) ? 0x1F : 0x0F;
    SET_DB(DS_InstrumentCount, count);
    uint16_t di = 0;
    do {
        si = (uint16_t)(si + 0x16);
        uint16_t w = rd16(mseg, si); si = (uint16_t)(si + 2);
        SET_DW(DS_InstrumentTable + di, (uint16_t)((uint16_t)(w << 8 | w >> 8) << 1));
        w = rd16(mseg, si); si = (uint16_t)(si + 2);
        SET_DW(DS_word_0BD4 + di, (uint16_t)(w << 8 | w >> 8));
        w = rd16(mseg, si); si = (uint16_t)(si + 2);
        SET_DW(DS_word_0BD6 + di, (uint16_t)((uint16_t)(w << 8 | w >> 8) << 1));
        w = rd16(mseg, si); si = (uint16_t)(si + 2);
        SET_DW(DS_word_0BD8 + di, (uint16_t)((uint16_t)(w << 8 | w >> 8) << 1));
        di = (uint16_t)(di + 0x10);
        count--;
    } while (count != 0);

    uint8_t songlen = rd8(mseg, si);
    si = (uint16_t)(si + 1);
    SET_DB(DS_SongLength, songlen);
    si = (uint16_t)(si + 1);                     /* restart byte */
    for (uint16_t k = 0; k < 0x40; k++) {        /* rep movsw: the 128-byte order table */
        SET_DW(DS_OrderTable + 2 * k, rd16(mseg, si));
        si = (uint16_t)(si + 2);
    }
    if (DB(DS_InstrumentCount) != 0x0F)          /* 31-sample modules: skip the M.K. signature */
        si = (uint16_t)(si + 4);

    /* Highest pattern in the order table, +1. CX = SongLength: 0 means 65536 passes. */
    uint8_t maxpat = 0;
    uint32_t passes = songlen ? songlen : 0x10000u;
    uint16_t p = DS_OrderTable;
    for (uint32_t i = 0; i < passes; i++) {
        uint8_t v = DB(p);
        if (v > maxpat) maxpat = v;
        p = (uint16_t)(p + 1);
    }
    maxpat = (uint8_t)(maxpat + 1);
    SET_DB(DS_PatternCount, maxpat);
    SET_DW(DS_PatternDataOffset, si);

    /* Sample data: segment of each sample (patterns come first, 64 paragraphs each); the
     * bytes are rescaled in place to (b + 80h) >> 2. */
    uint16_t seg = (uint16_t)((uint16_t)maxpat << 6) + mseg;
    uint16_t bx = 0;
    uint16_t instruments = DB(DS_InstrumentCount);
    for (uint16_t k = 0; k < instruments; k++) {
        uint16_t offset = si;
        si &= 0x0F;
        SET_DW(DS_word_0BDA + bx, si);
        seg = (uint16_t)(seg + (offset >> 4));
        SET_DW(DS_word_0BDC + bx, seg);
        uint16_t bp = DW(DS_InstrumentTable + bx);
        if (bp != 0) {
            uint16_t d = si;
            do {
                uint8_t v = rd8(seg, d);
                wr8(seg, d, (uint8_t)((uint8_t)(v + 0x80) >> 2));
                d = (uint16_t)(d + 1);
                bp--;
            } while (bp != 0);
        }
        si = (uint16_t)(si + DW(DS_InstrumentTable + bx));
        bx = (uint16_t)(bx + 0x10);
    }

    /* Pattern cells: each cell's first word (period) becomes its note index (period table
     * at DATA:0E75h, 36 entries; 36 when not found). PatternCount * 256 cells in all. */
    si = DW(DS_PatternDataOffset);
    uint16_t cells = (uint16_t)((uint16_t)DB(DS_PatternCount) << 8);
    do {
        uint16_t w = rd16(mseg, si);
        si = (uint16_t)(si + 2);
        if (w != 0) {
            uint16_t key = (uint16_t)((((w & 0x0Fu)) << 8) | (w >> 8));
            uint16_t cx = 0x24, dptr = 0x0E75;
            while (cx != 0) {                    /* repne scasw over DATA:0E75h */
                uint16_t t = DW(dptr);
                dptr = (uint16_t)(dptr + 2);
                cx--;
                if (t == key) break;
            }
            wr16(mseg, (uint16_t)(si - 2), (uint16_t)(0x24 - cx));
        }
        si = (uint16_t)(si + 2);
        cells--;
    } while (cells != 0);

    SET_DW(DS_SongOrderPos, 0);
    SET_DW(DS_SongRow, 0);
    SET_DB(DS_SongTickCount, 1);
    SET_DB(DS_SongSpeed, 6);
    for (uint16_t k = 0; k < 4; k++) SET_DW(0x0B8Au + 2 * k, 0xFFFF);   /* four channel positions: none */
    for (uint16_t k = 0; k < 32; k++) SET_DW(0x0B92u + 2 * k, 0);       /* channel fields up to InstrumentTable */
    return resource_sound_update();
}

int ActivateResourceSlot(uint16_t slot)
{
    int st = SYS_OK;
    SET_DW(DS_NextFreeSegment, (uint16_t)(DW(DS_NextFreeSegment) + 0x80));
    if (CB(CS_SbDetected) != 0 && !(DB(RF(slot)) & 1) && DB(DS_byte_2879) != 1) {
        if (CW(CS_OptionBits) & 0x40) st = resource_sound_update();
        else st = activate_slot_body(slot);
    }
    SET_DW(DS_NextFreeSegment, (uint16_t)(DW(DS_NextFreeSegment) - 0x80));
    return st;
}

/* ---- palettes (cs:0B8Ch..0BEEh) -------------------------------------------------------- */

void SetDacBlock0To15IfFlag(uint16_t seg, uint16_t dx)
{
    if (CB(CS_OptionVByte) == 5)                 /* never 5 in the shipped paths (see ASM) */
        platform_dac_set_block(seg, dx, 0, 0x10);
}

void SetStartupPalette(void)
{
    SetDacBlock0To15IfFlag(DATA_SEG, 0x287Eu);
}

void SetLevelPalette(void)
{
    uint16_t bx = (uint16_t)(DB(DS_CurrentLevel) << 1);
    uint16_t dx = DW((uint16_t)(DS_LevelPaletteTable + bx));
    platform_dac_set_block(DATA_SEG, dx, 0, 0x10);
}

/* ---- text (cs:0BF7h..0CB2h) -------------------------------------------------------------- */

void WaitForButtonPressRelease(void)
{
    do PollInputState(); while ((DB(DS_byte_2832) | DB(DS_byte_27E8)) == 0);
    do PollInputState(); while ((DB(DS_byte_27E8) | DB(DS_byte_2832)) != 0);
}

void PrintDecimalWord(uint16_t dx)
{
    SET_DB(DS_byte_27D5, 0);
    uint16_t bx = 0x2710;                        /* 10000, 1000, ..., 1 */
    do {
        uint8_t al = 0x30;
        while (dx >= bx) { dx = (uint16_t)(dx - bx); al++; }
        if (al == 0x30 && bx != 1 && DB(DS_byte_27D5) == 0) {
            al = 0x20;                           /* leading zero: a space */
        } else {
            SET_DB(DS_byte_27D5, 1);
        }
        DrawFontChar(al);
        bx = (uint16_t)(bx / 10);
    } while (bx != 0);
}

uint16_t PrintAsciiz(uint16_t seg, uint16_t si)
{
    for (;;) {
        uint8_t c = rd8(seg, si);
        si = (uint16_t)(si + 1);
        if (c == 0) return si;
        DrawFontChar(c);
    }
}

void DrawFontChar(uint8_t al)
{
    uint16_t di = (uint16_t)(DW(DS_TextRowOffset) + DW(DS_TextByteColumn) + DW(DS_DisplayPage));
    SET_DW(DS_TextByteColumn, (uint16_t)(DW(DS_TextByteColumn) + 1));
    if (al == 0x20) {
        /* a space clears the 12-row column in all four planes (map mask 0Fh) */
        SetCpuWriteMode();
        port_out16(0x3C4, 0x0F02);
        for (int row = 0; row < 12; row++) {
            vga_write(di, 0);
            di = (uint16_t)(di + 0x28);          /* stosb, then add di, 27h: 40 bytes per row */
        }
        return;
    }
    /* glyph index as the ASM computes it: AL - 30h, and 2 less again past the digits */
    uint8_t g = (uint8_t)(al - 0x30);
    if (g > 9) g = (uint8_t)(g - 2);
    uint16_t si = (uint16_t)(g * 0x30);         /* 48 bytes per glyph: 12 rows x 4 planes */
    uint16_t fseg = DW(DS_word_003D);           /* the ALLFONTS segment */
    SetCpuWriteMode();
    port_out8(0x3C4, 2);
    uint8_t plane = 1;
    do {
        port_out8(0x3C5, plane);
        for (int row = 0; row < 12; row++) {
            vga_write(di, rd8(fseg, si));
            si = (uint16_t)(si + 1);
            di = (uint16_t)(di + 0x28);
        }
        di = (uint16_t)(di - 0x1E0);             /* back to the top row for the next plane */
        plane = (uint8_t)(plane << 1);
    } while (plane <= 8);
}

/* ---- joystick and input (cs:0C80h..0F7Eh) --------------------------------------------- */

void CalibrateJoystick(void)
{
    SET_DB(DS_byte_27E5, 3);
    for (;;) {
        SET_DB(DS_byte_27D9, 0);
        SET_DW(DS_word_27DA, 0x320);
        uint8_t ah = 3;
        for (uint16_t cx = 0x0C80; cx != 0; cx--) {   /* sample which line pair toggles */
            uint8_t al = port_in8(JOY_PORT);
            if (al & 3) ah &= 2;
            if (al & 0x0C) ah &= 1;
        }
        uint8_t al = 0xFF;
        if (ah & 2) al = DB(DS_byte_27E7);
        if (ah & 1) al = DB(DS_byte_27E6);
        SET_DB(DS_byte_27E4, al);

        uint16_t x = 0, y = 0;
        if (!ReadJoystickTimes(&x, &y) && x < 0x320 && y < 0x320 && x != 0 && y != 0) {
            SET_DW(DS_word_27DC, (uint16_t)(x - (x >> 2)));  /* X low limit */
            uint16_t hx = (uint16_t)((x >> 1) + x);          /* X high limit */
            SET_DW(DS_word_27E2, hx);
            SET_DW(DS_word_27DE, (uint16_t)(y - (y >> 2)));  /* Y low limit */
            uint16_t hy = (uint16_t)((y >> 2) + y);          /* Y high limit */
            SET_DW(DS_word_27E0, hy);
            uint16_t m = hx;
            if (hx < hy) m = hy;                             /* jae: keep the larger */
            SET_DW(DS_word_27DA, (uint16_t)(m << 1));
            return;
        }
        /* loc_0D51: retry with the two candidate line masks swapped, three attempts */
        SET_DB(DS_byte_27E5, (uint8_t)(DB(DS_byte_27E5) - 1));
        if (DB(DS_byte_27E5) == 0) {
            SET_DB(DS_byte_27E4, 0xFF);
            return;
        }
        uint8_t t = DB(DS_byte_27E6);
        SET_DB(DS_byte_27E6, DB(DS_byte_27E7));
        SET_DB(DS_byte_27E7, t);
    }
}

int ReadJoystickTimes(uint16_t *out_x, uint16_t *out_y)
{
    uint8_t cl = DB(DS_byte_27E4);
    if (cl & 0x80) return 1;                     /* FFh: no joystick */
    if (DB(DS_byte_27D9) != 0) return 1;
    uint8_t al = shr8(port_in8(JOY_PORT), cl);
    if (al & 3) return 1;                        /* the lines must start low */
    al = (uint8_t)((uint8_t)(al ^ 0x30) >> 4);   /* two button bits, active high */
    unsigned cf = al & 1;
    al = (uint8_t)(al >> 1);
    SET_DB(DS_byte_27E8, (uint8_t)(DB(DS_byte_27E8) - cf));   /* sbb with BH = 0 */
    cf = al & 1;
    al = (uint8_t)(al >> 1);
    SET_DB(DS_byte_27E9, (uint8_t)(DB(DS_byte_27E9) - cf));
    uint16_t si = DW(DS_word_27DA);
    port_out8(JOY_PORT, al);                     /* fire the one-shot */
    uint16_t x = 0, y = 0;
    do {
        uint8_t v = shr8(port_in8(JOY_PORT), cl);
        if ((v & 3) == 0) break;
        x = (uint16_t)(x + (v & 1));             /* X line high: bit 0 */
        y = (uint16_t)(y + ((v >> 1) & 1));      /* Y line high: bit 1 */
        si--;
    } while (si != 0);
    *out_x = x;
    *out_y = y;
    return 0;
}

void PollInputState(void)
{
    if (DB(DS_byte_2879) != 0) {
        /* demo playback: one (value, count) entry of the log at DATA:003Fh per frame */
        if (DB(DS_LastScanCode) != 0) SET_DB(DS_EndSignal, 1);
        uint16_t si = DW(DS_word_287A);
        uint8_t al;
        if (si != 0 && DB(DS_byte_287D) != 0) {
            SET_DB(DS_byte_287D, (uint8_t)(DB(DS_byte_287D) - 1));
            al = DB(DS_byte_287C);
        } else {
            uint16_t ax = DW((uint16_t)(si + DS_word_003F));
            si = (uint16_t)(si + 2);
            if (ax == 0x55AA) SET_DB(DS_EndSignal, 1);
            SET_DW(DS_word_287A, si);
            SET_DB(DS_byte_287C, (uint8_t)ax);
            SET_DB(DS_byte_287D, (uint8_t)(ax >> 8));
            al = (uint8_t)ax;
        }
        SET_DB(DS_byte_27ED, al & 1);
        SET_DB(DS_byte_27EC, (al >> 1) & 1);
        SET_DB(DS_byte_27EA, (al >> 2) & 1);
        SET_DB(DS_byte_27EB, (al >> 3) & 1);
        SET_DB(DS_byte_27E8, (al >> 4) & 1);
        SET_DB(DS_byte_27E9, (al >> 5) & 1);
    }
    if (DB(DS_byte_2879) == 1) return;           /* playback only */

    /* live sampling: joystick directions and buttons, then the keyboard flag groups */
    SET_DB(DS_byte_27EA, 0);
    SET_DB(DS_byte_27EB, 0);
    SET_DB(DS_byte_27EC, 0);
    SET_DB(DS_byte_27ED, 0);
    SET_DB(DS_byte_27E8, 0);
    SET_DB(DS_byte_27E9, 0);
    uint16_t x = 0, y = 0;
    if (!ReadJoystickTimes(&x, &y)) {
        if (x <= DW(DS_word_27DC)) SET_DB(DS_byte_27ED, 0xFF);
        else if (x >= DW(DS_word_27E2)) SET_DB(DS_byte_27EC, 0xFF);
        if (y <= DW(DS_word_27DE)) SET_DB(DS_byte_27EA, 0xFF);
        else if (y >= DW(DS_word_27E0)) SET_DB(DS_byte_27EB, 0xFF);
    }
    SET_DB(DS_byte_27EA, DB(DS_byte_27EA) | DB(DS_byte_283C) | DB(DS_byte_2870) | DB(DS_byte_281D) | DB(DS_byte_2804));
    SET_DB(DS_byte_27EB, DB(DS_byte_27EB) | DB(DS_byte_2844) | DB(DS_byte_286F) | DB(DS_byte_283E) | DB(DS_byte_2812));
    SET_DB(DS_byte_27EC, DB(DS_byte_27EC) | DB(DS_byte_2841) | DB(DS_byte_286D) | DB(DS_byte_2842) | DB(DS_byte_281B));
    SET_DB(DS_byte_27ED, DB(DS_byte_27ED) | DB(DS_byte_283F) | DB(DS_byte_286E) | DB(DS_byte_281F) | DB(DS_byte_281A));
    SET_DB(DS_byte_27E8, DB(DS_byte_27E8) | DB(DS_byte_2810) | DB(DS_byte_282D) | DB(DS_byte_2868) | DB(DS_byte_284B));
    SET_DB(DS_byte_27E9, DB(DS_byte_27E9) | DB(DS_byte_2840));

    /* record this frame into the log while it has room (bits packed as the playback reads them) */
    uint16_t si = DW(DS_word_287A);
    if (si >= 0x7FC) return;
    uint8_t al = 0;
    al = (uint8_t)((al << 1) | (DB(DS_byte_27E9) & 1));
    al = (uint8_t)((al << 1) | (DB(DS_byte_27E8) & 1));
    al = (uint8_t)((al << 1) | (DB(DS_byte_27EB) & 1));
    al = (uint8_t)((al << 1) | (DB(DS_byte_27EA) & 1));
    al = (uint8_t)((al << 1) | (DB(DS_byte_27EC) & 1));
    al = (uint8_t)((al << 1) | (DB(DS_byte_27ED) & 1));
    if (si == 0) {
        SET_DB(DS_byte_083E, DB(DS_CurrentLevel));
    } else if (DB((uint16_t)(si + DS_word_003D)) == al && DB((uint16_t)(si + DS_byte_003E)) != 0xFF) {
        SET_DB((uint16_t)(si + DS_byte_003E), DB((uint16_t)(si + DS_byte_003E)) + 1);   /* same frame: count it */
        return;
    }
    SET_DB((uint16_t)(si + DS_word_003F), al);
    SET_DB((uint16_t)(si + DS_byte_0040), 0);
    SET_DW(DS_word_287A, (uint16_t)(si + 2));
}

/* ---- interrupt vectors and the keyboard (cs:177Fh, cs:1823h) ------------------------- */

void SwapInt08Int09Vectors(void)
{
    SET_DW(DS_word_2381, rd16(0, 0x0E));         /* INT 03h segment, written and never read */
    for (uint16_t k = 0; k < 4; k++) {
        uint16_t ivt = rd16(0, (uint16_t)(0x20 + 2 * k));
        uint16_t slot = CW((uint16_t)(CS_Int08SwapSlot + 2 * k));
        SET_CW((uint16_t)(CS_Int08SwapSlot + 2 * k), ivt);
        wr16(0, (uint16_t)(0x20 + 2 * k), slot);
    }
    for (uint16_t k = 0; k < 0x40; k++) SET_DW(DS_KeyDownTable + 2 * k, 0);
}

int KeyboardIsr(void)
{
    uint8_t code = port_in8(KBD_DATA_PORT);
    uint8_t ctl = port_in8(KBD_CTRL_PORT);
    port_out8(KBD_CTRL_PORT, (uint8_t)(ctl | 0x80));   /* pulse the acknowledge bit */
    port_out8(KBD_CTRL_PORT, ctl);
    SET_DB(DS_LastScanCode, code);
    if (code == ESC_SCAN && DB(DS_DosCallActive) == 0)
        return SYS_QUIT;                         /* quit path: boundary, nothing of it is run */

    if (code == F3_SCAN) {                       /* F3 toggles the sound-on bit */
        SET_CB(CS_SoundState, CB(CS_SoundState) ^ 1);
        if (CB(CS_SbDetected) == 1) {
            if (CB(CS_SoundState) & 1) return SYS_UNSUPPORTED;   /* StartSbBlock: boundary */
        } else if (CB(CS_AdLibEnabled) != 1) {
            uint8_t al = (uint8_t)(port_in8(KBD_CTRL_PORT) & 0xFC);
            if (CB(CS_SoundState) & 1) al |= 3;
            port_out8(KBD_CTRL_PORT, al);
        }
    }
    /* KeyDownTable[code & 7Fh] = FFh on make, 00h on break */
    uint8_t last = DB(DS_LastScanCode);
    SET_DB((uint16_t)(DS_KeyDownTable + (code & 0x7F)), (last & 0x80) ? 0x00 : 0xFF);
    port_out8(PIC_EOI_PORT, 0x20);
    return SYS_OK;
}

/* ---- differential-test adapters (register contract -> C arguments) -------------------- */

EXPORT int c_StartSoundEffect(Regs *r) { StartSoundEffect(r->dx); return 0; }
EXPORT int c_ActivateResourceSlot(Regs *r) { return ActivateResourceSlot(r->ax); }
EXPORT int c_PageInResource(Regs *r) { return PageInResource(r->cx); }
EXPORT int c_ResourceNameAt(Regs *r) { r->dx = ResourceNameAt(r->cx); return 0; }
EXPORT int c_SetStartupPalette(Regs *r) { SetStartupPalette(); r->ds = DATA_SEG; r->es = DATA_SEG; return 0; }
EXPORT int c_SetLevelPalette(Regs *r) { SetLevelPalette(); r->ds = DATA_SEG; r->es = DATA_SEG; return 0; }
EXPORT int c_SetDacBlock0To15IfFlag(Regs *r) { SetDacBlock0To15IfFlag(r->es, r->dx); return 0; }
EXPORT int c_WaitForButtonPressRelease(Regs *r) { (void)r; WaitForButtonPressRelease(); return 0; }
EXPORT int c_PrintDecimalWord(Regs *r) { PrintDecimalWord(r->dx); return 0; }
EXPORT int c_PrintAsciiz(Regs *r) { r->si = PrintAsciiz(r->ds, r->si); return 0; }
EXPORT int c_DrawFontChar(Regs *r) { DrawFontChar((uint8_t)r->ax); return 0; }
EXPORT int c_CalibrateJoystick(Regs *r) { (void)r; CalibrateJoystick(); return 0; }
EXPORT int c_PollInputState(Regs *r) { (void)r; PollInputState(); return 0; }
EXPORT int c_SwapInt08Int09Vectors(Regs *r) { (void)r; SwapInt08Int09Vectors(); return 0; }

EXPORT int c_ReadJoystickTimes(Regs *r)
{
    uint16_t x = r->ax, y = r->bx;               /* kept on failure (AL, the port scratch, is not reproduced) */
    int cf = ReadJoystickTimes(&x, &y);
    r->ax = x; r->bx = y;
    r->flags = cf ? (uint16_t)(r->flags | FLAG_CF) : (uint16_t)(r->flags & ~FLAG_CF);
    return 0;
}

/* ZF carries the quit boundary: at loc_1850 the ASM has ZF = 1 from the DosCallActive
 * compare, so the adapter reports the boundary through the same flag. */
EXPORT int c_KeyboardIsr(Regs *r)
{
    int s = KeyboardIsr();
    if (s == SYS_UNSUPPORTED) return s;
    r->flags = s == SYS_QUIT ? (uint16_t)(r->flags | FLAG_ZF) : (uint16_t)(r->flags & ~FLAG_ZF);
    return 0;
}
