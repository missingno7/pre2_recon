/* SQZ resource loader and decoders (oracle: sub_107B and its paths in src/MODULE1.ASM).
 *
 * LoadSqzFile(name) opens the file at DS:name, allocates its decompressed size at the
 * free-paragraph pointer (DATA word_2875) and decodes it there with one of three codecs,
 * chosen by the first header bytes:
 *   - word 4CB4h:  LZSS bit stream (17-byte header; sub_14D8),
 *   - byte 1 = 10h: LZW, 9..12-bit MSB-first codes, 100h = clear, 101h = end,
 *   - otherwise:   Huffman tree + run lengths (6-byte header, tree follows).
 * Work areas live in unallocated memory right after the new allocation and in the
 * code segment (cvar_1204 handle, cvar_1206 output segment, cvar_1409.. LZSS refill
 * state), exactly as in the original, so the memory left behind is identical.
 * byte_2877 (DATA) is the "inside DOS" flag the timer interrupt checks; it is raised
 * around each DOS call. The fatal paths (messages, wait, retry) are platform services.
 */
#include "pre2.h"
#include "dos_files.h"
#include "sqz.h"

#define IN_DOS        DS_BYTE_2877
#define FREE_SEG      DS_WORD_2875
#define MEM_TOP       CS_CVAR_0006      /* paragraphs of conventional memory (INT 12h << 6) */
#define FILE_HANDLE   CS_CVAR_1204
#define OUT_SEG       CS_CVAR_1206
#define LZ_SRC_MODE   CS_CVAR_140F      /* 0 file, 1 memory, 2 EMS, 3 XMS */
#define LZ_BUF_BYTES  CS_CVAR_1410
#define LZ_REFILL_AT  CS_CVAR_1412

static void dos_enter(void) { SET_DB(IN_DOS, 1); }
static void dos_leave_dec(void) { SET_DB(IN_DOS, DB(IN_DOS) - 1); }

/* 32-bit (hi:lo) >> 4, +1: the paragraph count the original allocates for a size. */
static uint16_t paragraphs(uint16_t hi, uint16_t lo) { return (uint16_t)((((uint32_t)hi << 16 | lo) >> 4) + 1); }

static int allocate(uint16_t paras, uint16_t *seg)
{
    uint16_t top = (uint16_t)(DW(FREE_SEG) + paras);
    SET_DW(FREE_SEG, top);
    *seg = top;
    return CW(MEM_TOP) > top ? 0 : SQZ_FATAL_MEMORY;
}

/* ---- Huffman tree + run lengths (loc_10D7 .. loc_11E5) -------------------------- */

typedef struct { uint16_t ws, ax; uint8_t dl; uint16_t si; } HuffBits;

/* sub_11D0: walk the tree from node offset bx; returns the leaf (bit 15 cleared) or
 * -1 when the file ended during a refill (the original then leaves the decoder). */
static int32_t huff_symbol(HuffBits *b, uint16_t bx)
{
    for (;;) {
        if (--b->dl == 0) {                                     /* loc_1190 */
            uint16_t left = (uint16_t)(rd16(b->ws, 0) - 2);
            wr16(b->ws, 0, left);
            if (left == 0) {
                uint16_t n;
                dos_enter();
                dos_read(CW(FILE_HANDLE), b->ws, 0x041A, 0x2800, &n);
                dos_leave_dec();
                wr16(b->ws, 0, n);
                if (n == 0) return -1;
                wr16(b->ws, 0, (uint16_t)(n + (n & 1)));
                b->si = 0x041A;
            }
            uint16_t w = rd16(b->ws, b->si); b->si += 2;           /* loc_11CB */
            b->ax = (uint16_t)(w >> 8 | w << 8);
            b->dl = 0x10;
        }
        int bit = b->ax >> 15;
        b->ax = (uint16_t)(b->ax << 1 | bit);
        if (bit) bx += 2;
        bx = rd16(b->ws, (uint16_t)(bx + 6));
        if (bx & 0x8000) return bx & 0x7FFF;
    }
}

static int decode_huffman(uint16_t out, uint16_t hdr0, uint16_t hdr1, uint16_t tree_bytes)
{
    uint16_t ws, n;
    if (allocate(paragraphs(hdr0, hdr1), &ws)) return SQZ_FATAL_MEMORY;
    dos_enter();
    if (dos_read(CW(FILE_HANDLE), ws, 6, tree_bytes, &n)) return SQZ_FATAL_READ;
    dos_leave_dec();
    HuffBits b = { ws, 0, 1, 0 };
    wr16(ws, 0, 2);
    uint32_t dst = lin(out, 0);
    for (;;) {
        int32_t s = huff_symbol(&b, 0);
        if (s < 0) break;
        uint16_t count;
        if ((s >> 8) == 0) { pre2_mem[dst++] = (uint8_t)s; continue; }
        if ((s & 0xFF) >= 2) count = s & 0xFF;
        else if ((s & 0xFF) == 0) {
            int32_t c = huff_symbol(&b, 0);
            if (c < 0) break;
            count = (uint16_t)c;
        } else {
            int32_t hi = huff_symbol(&b, 0);
            if (hi < 0) break;
            int32_t lo = huff_symbol(&b, 0);
            if (lo < 0) break;
            count = (uint16_t)((hi & 0xFF) << 8 | (lo & 0xFF));
        }
        uint8_t prev = pre2_mem[dst - 1];   /* es:[di-1] */
        while (count--) pre2_mem[dst++] = prev;
    }
    uint16_t ax;
    dos_enter(); dos_close(CW(FILE_HANDLE), &ax); dos_leave_dec();
    return 0;
}

/* ---- LZW (loc_1208 .. loc_131E, sub_133C, sub_134E) ----------------------------- */
/* Work segment layout (offsets in the segment right after the allocation):
 *   0 valid bits in the window, 2 bit position, 4 first byte of the last string,
 *   5 previous code, 7 current code, 9 code width, 0Ah next width threshold,
 *   0Ch next free code, 0Eh.. bit window (2 carried bytes + 1C00h read buffer),
 *   1C10h suffix bytes, 2C10h prefix words, 4C10h output stack. */

static uint16_t lzw_code(uint16_t ws, uint32_t *dst, uint16_t out)
{
    if (rd16(ws, 0x0A) == rd16(ws, 0x0C) && rd8(ws, 9) != 12) {
        wr8(ws, 9, (uint8_t)(rd8(ws, 9) + 1));
        wr16(ws, 0x0A, (uint16_t)(rd16(ws, 0x0A) << 1));
    }
    uint16_t width = rd8(ws, 9);
    uint16_t pos = rd16(ws, 2);
    if (rd16(ws, 0) < (uint16_t)(pos + width)) {
        pos = (uint16_t)(0x10 - rd16(ws, 0) + rd16(ws, 2));
        wr16(ws, 2, pos);
        wr16(ws, 0x0E, rd16(ws, 0x1C0E));
        uint16_t n;
        dos_enter();
        dos_read(CW(FILE_HANDLE), ws, 0x10, 0x1C00, &n);
        dos_leave_dec();
        wr16(ws, 0, (uint16_t)((n + 2) << 3));
        (void)dst; (void)out;      /* the original renormalizes ES:DI here; linear output is unchanged */
    }
    uint16_t si = pos >> 3;
    uint8_t cl = pos & 7;
    uint16_t w = rd16(ws, (uint16_t)(si + 0x0E));
    uint16_t ax = (uint16_t)((uint16_t)(w >> 8 | w << 8) << cl);
    uint8_t dh = 0;
    int ch = 16 - cl - width;
    if (ch < 0) dh = (uint8_t)(rd8(ws, (uint16_t)(si + 0x10)) >> (8 + ch));
    wr16(ws, 2, (uint16_t)(rd16(ws, 2) + width));
    ax = (uint16_t)(ax >> (16 - width));
    return (uint16_t)((ax & 0xFF00) | ((ax | dh) & 0xFF));
}

static int decode_lzw(uint16_t out)
{
    uint32_t pos;
    uint16_t ax, ws, n;
    dos_enter();
    dos_seek(CW(FILE_HANDLE), 0, 0, &pos);
    SET_DB(IN_DOS, 0);
    dos_enter();
    if (dos_read(CW(FILE_HANDLE), out, 0x0E, 4, &n)) return SQZ_FATAL_READ;
    SET_DB(IN_DOS, 0);
    if ((rd8(out, 0x0F) & 0xF0) != 0x10) return SQZ_LZSS_INSTEAD;
    if (allocate(paragraphs(rd16(out, 0x0E), rd16(out, 0x10)), &ws)) return SQZ_FATAL_MEMORY;
    wr16(ws, 0, 0); wr16(ws, 2, 0);
    uint32_t dst = lin(out, 0);
    for (;;) {
        wr16(ws, 0x0A, 0x200); wr8(ws, 9, 9); wr16(ws, 0x0C, 0x102);       /* sub_133C */
        ax = lzw_code(ws, &dst, out);
        if (ax == 0x101) break;
        wr16(ws, 5, ax); wr8(ws, 4, (uint8_t)ax);
        pre2_mem[dst++] = (uint8_t)ax;
        for (;;) {
            ax = lzw_code(ws, &dst, out);
            if (ax == 0x100) break;                 /* clear: restart the dictionary */
            if (ax == 0x101) goto done;
            wr16(ws, 7, ax);
            uint16_t sp = 0x4C10;
            if (rd16(ws, 0x0C) <= ax) {             /* code not yet in the dictionary */
                wr8(ws, sp++, rd8(ws, 4));
                ax = rd16(ws, 5);
            }
            while (ax >> 8) {
                wr8(ws, sp++, rd8(ws, (uint16_t)(ax + 0x1C10)));
                ax = rd16(ws, (uint16_t)(0x2C10 + 2*ax));
            }
            wr8(ws, sp++, (uint8_t)ax);
            wr8(ws, 4, (uint8_t)ax);
            do pre2_mem[dst++] = rd8(ws, --sp); while (sp != 0x4C10);
            uint16_t next = rd16(ws, 0x0C);
            if (next < 0x1000) {
                wr8(ws, (uint16_t)(next + 0x1C10), rd8(ws, 4));
                wr16(ws, (uint16_t)(0x2C10 + 2*next), rd16(ws, 5));
                wr16(ws, 0x0C, (uint16_t)(next + 1));
            }
            wr16(ws, 5, rd16(ws, 7));
        }
    }
done:
    dos_enter(); dos_close(CW(FILE_HANDLE), &ax); dos_leave_dec();
    return 0;
}

/* ---- LZSS (loc_1414 .. sub_1700) --------------------------------------------------- */

typedef struct { uint16_t ds, si, bp; uint8_t dl; uint32_t dst; int fatal; } LzBits;

/* sub_1700 mode 0 (file): refill the input buffer when SI passed the threshold. */
static void lz_refill(LzBits *b)
{
    if (CB(LZ_SRC_MODE) >= 2) { b->fatal = SQZ_UNSUPPORTED; return; }   /* EMS/XMS sources: not translated yet */
    if (CB(LZ_SRC_MODE) == 0 && b->si >= CW(LZ_REFILL_AT)) {
        uint16_t keep = (uint16_t)(CW(LZ_REFILL_AT) + 0x80 - b->si);
        for (uint16_t i = 0; i < keep; i++) wr8(b->ds, i, rd8(b->ds, (uint16_t)(b->si + i)));
        uint16_t n;
        SET_DB(IN_DOS, DB(IN_DOS) + 1);
        if (dos_read(CW(FILE_HANDLE), b->ds, keep, (uint16_t)(CW(LZ_BUF_BYTES) - keep), &n)) { b->fatal = SQZ_FATAL_SQUEEZED; return; }
        dos_leave_dec();
        b->si = 0;
    }
    b->bp = rd16(b->ds, b->si); b->si += 2;
    b->dl = 0x10;
}

static int lz_bit(LzBits *b)
{
    int bit = b->bp & 1;
    b->bp >>= 1;
    if (--b->dl == 0) lz_refill(b);
    return bit;
}

static void lz_copy(LzBits *b, uint16_t dist, uint16_t count)
{
    while (count--) { pre2_mem[b->dst] = pre2_mem[b->dst + (int16_t)dist]; b->dst++; }
}

/* sub_14D8 core: DS:SI = compressed stream, ES:DI = output. Returns 0 at the end code. */
static int lz_decode(LzBits *b)
{
    b->dl = 0x10;
    b->bp = rd16(b->ds, b->si); b->si += 2;
    for (;;) {
        if (lz_bit(b)) { pre2_mem[b->dst++] = rd8(b->ds, b->si++); if (b->fatal) return b->fatal; continue; }
        int long_form = lz_bit(b);
        uint8_t bl = rd8(b->ds, b->si++), bh = 0xFF;
        uint16_t count;
        if (!long_form) {
            if (!lz_bit(b)) {
                if (bl == bh) return b->fatal;           /* end code */
                count = 2;
            } else {
                for (int i = 0; i < 3; i++) bh = (uint8_t)(bh << 1 | lz_bit(b));
                bh--;
                count = 2;
            }
        } else {
            bh = (uint8_t)(bh << 1 | lz_bit(b));
            if (!lz_bit(b)) {
                uint8_t dh = 2;
                for (int i = 0; i < 3; i++) {
                    if (lz_bit(b)) break;
                    bh = (uint8_t)(bh << 1 | lz_bit(b));
                    dh = (uint8_t)(dh + dh);
                }
                bh = (uint8_t)(bh - dh);
            }
            uint8_t dh = 2;
            int done = 0;
            for (int i = 0; i < 4 && !done; i++) { dh++; if (lz_bit(b)) done = 1; }
            if (done) count = dh;
            else if (lz_bit(b)) { dh++; dh = (uint8_t)(dh + lz_bit(b)); count = dh; }
            else if (lz_bit(b)) count = (uint16_t)(rd8(b->ds, b->si++) + 0x11);
            else {
                uint8_t v = 0;
                for (int i = 0; i < 3; i++) v = (uint8_t)(v << 1 | lz_bit(b));
                count = (uint16_t)(v + 9);
            }
        }
        if (b->fatal) return b->fatal;
        lz_copy(b, (uint16_t)(bh << 8 | bl), count);
    }
}

static int decode_lzss(uint16_t out)
{
    uint32_t pos;
    uint16_t n, ws;
    dos_enter();
    if (dos_seek(CW(FILE_HANDLE), 0, 0, &pos)) return SQZ_FATAL_SQUEEZED;
    dos_leave_dec();
    SET_DB(IN_DOS, DB(IN_DOS) + 1); dos_read(CW(FILE_HANDLE), out, 0, 0x11, &n); dos_leave_dec();
    if (rd16(out, 4) != 0x899D || rd16(out, 6) != 0x6C64) return SQZ_FATAL_SQUEEZED;
    uint8_t dl = rd8(out, 0x0E) >> 2;
    uint16_t paras = (uint16_t)(((((uint32_t)dl << 16) | rd16(out, 0x0F)) >> 4) + 1);
    ws = (uint16_t)(DW(FREE_SEG) + paras);
    SET_DW(FREE_SEG, ws);
    if (ws >= CW(MEM_TOP)) return SQZ_FATAL_MEMORY;
    uint16_t avail = (uint16_t)(CW(MEM_TOP) - ws), bytes = 0xF000;
    if (avail < 0x0F00) bytes = (uint16_t)(avail << 4);
    SET_CW(LZ_BUF_BYTES, bytes);
    SET_CW(LZ_REFILL_AT, (uint16_t)(bytes - 0x80));
    SET_DB(IN_DOS, DB(IN_DOS) + 1);
    if (dos_read(CW(FILE_HANDLE), ws, 0, bytes, &n)) return SQZ_FATAL_SQUEEZED;
    dos_leave_dec();
    LzBits b = { ws, 0, 0, 0, lin(out, 0), 0 };
    int r = lz_decode(&b);
    if (r) return r;
    if (CB(LZ_SRC_MODE) == 0) {
        uint16_t ax;
        dos_enter(); dos_close(CW(FILE_HANDLE), &ax); dos_leave_dec();
    }
    return 0;
}

/* ---- sub_107B ------------------------------------------------------------------------ */

int LoadSqzFile(uint16_t name, uint16_t *out_seg)
{
    uint16_t ax, n;
    dos_enter();
    int failed = dos_open(DATA_SEG, name, &ax);
    SET_DB(IN_DOS, 0);
    if (failed) return SQZ_FATAL_OPEN;
    SET_CW(FILE_HANDLE, ax);
    uint16_t out = DW(FREE_SEG);
    SET_CW(OUT_SEG, out);
    dos_enter();
    if (dos_read(CW(FILE_HANDLE), out, 0x041A, 6, &n)) return SQZ_FATAL_READ;
    dos_leave_dec();
    uint16_t w0 = rd16(out, 0x041A);
    int r;
    if (w0 == 0x4CB4) r = decode_lzss(out);
    else if ((w0 >> 8) == 0x10) {
        r = decode_lzw(out);
        if (r == SQZ_LZSS_INSTEAD) r = decode_lzss(out);
    } else r = decode_huffman(out, w0, rd16(out, 0x041C), rd16(out, 0x041E));
    if (r) return r;
    *out_seg = CW(OUT_SEG);
    return 0;
}

/* Differential-test adapter: DX = name offset in DATA; returns AX = output segment. */
EXPORT int c_sub_107B(Regs *r)
{
    uint16_t seg = 0;
    int e = LoadSqzFile(r->dx, &seg);
    r->ax = seg; r->ds = DATA_SEG;
    return e;
}
