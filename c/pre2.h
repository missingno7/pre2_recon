/* Original memory model shared by every translated region.
 *
 * The C code operates on the original real-mode address space: `pre2_mem` holds 1 MiB
 * (+64 KiB for segment:offset sums past FFFFF) laid out exactly as the DOS program sees
 * it, with the program image loaded at LOAD_SEG. Game state is addressed by its original
 * segment and 16-bit offset; offsets wrap at 64 KiB as in real mode. Symbol offsets come
 * from the generated pre2_sym.h (from src/, never written by hand). No C statics hold
 * game state.
 */
#ifndef PRE2_H
#define PRE2_H
#include <stdint.h>

#ifdef _WIN32
#define EXPORT __declspec(dllexport)   /* entry points used by the test harness */
#else
#define EXPORT
#endif

#define LOAD_SEG   0x1010u              /* PSP at 1000h */
#define CODE_SEG   (LOAD_SEG + 0x0020u)  /* the code segment frame */
#define DATA_SEG   (LOAD_SEG + 0x09FFu)  /* DS for all game code */
#define MEM_SIZE   0x110000u

extern uint8_t pre2_mem[MEM_SIZE];
/* The ES register where the original leaves it changed across routine boundaries (a register
 * model, not game state): each translated routine leaves pre2_es as the original leaves ES
 * on return, so a consumer of a stale ES (loc_01C4's render-slot fill) can use it. The
 * differential tests compare it with the oracle's ES after every case. */
extern uint16_t pre2_es;
#define SET_ES(v) (pre2_es = (uint16_t)(v))

static inline uint32_t lin(uint16_t seg, uint16_t off) { return ((uint32_t)seg << 4) + off; }
static inline uint8_t  rd8 (uint16_t seg, uint16_t off) { return pre2_mem[lin(seg, off)]; }
static inline uint16_t rd16(uint16_t seg, uint16_t off)
{ return (uint16_t)(rd8(seg, off) | rd8(seg, (uint16_t)(off + 1)) << 8); }
static inline void wr8 (uint16_t seg, uint16_t off, uint8_t v) { pre2_mem[lin(seg, off)] = v; }
static inline void wr16(uint16_t seg, uint16_t off, uint16_t v)
{ wr8(seg, off, (uint8_t)v); wr8(seg, (uint16_t)(off + 1), (uint8_t)(v >> 8)); }

/* DATA (ds) and code-segment (cs) shorthands */
#define DB(off)        rd8(DATA_SEG, (uint16_t)(off))
#define DW(off)        rd16(DATA_SEG, (uint16_t)(off))
#define SET_DB(off, v) wr8(DATA_SEG, (uint16_t)(off), (uint8_t)(v))
#define SET_DW(off, v) wr16(DATA_SEG, (uint16_t)(off), (uint16_t)(v))
#define CB(off)        rd8(CODE_SEG, (uint16_t)(off))
#define CW(off)        rd16(CODE_SEG, (uint16_t)(off))
#define SET_CB(off, v) wr8(CODE_SEG, (uint16_t)(off), (uint8_t)(v))
#define SET_CW(off, v) wr16(CODE_SEG, (uint16_t)(off), (uint16_t)(v))

/* Register file for routines entered through their original register contract
 * (differential-test adapters and ASM-shaped boundaries). */
typedef struct Regs {
    uint16_t ax, bx, cx, dx, si, di, bp, ds, es;
    uint16_t flags;   /* CF = bit 0, ZF = bit 6, SF = bit 7, OF = bit 11 */
} Regs;
#define FLAG_CF 0x0001u
#define FLAG_ZF 0x0040u

/* Register-contract helpers shared by the adapters and the ASM-shaped leaves. */
static inline void set_cf(Regs *r, int cf)
{
    r->flags = cf ? (uint16_t)(r->flags | FLAG_CF) : (uint16_t)(r->flags & ~FLAG_CF);
}

/* cbw: AL sign-extended to AX. */
static inline uint16_t cbw(uint8_t al) { return (uint16_t)(int16_t)(int8_t)al; }

/* ROL r16, n with the count masked to 4 bits (the rotation is modulo 16 on the 186+ CPUs). */
static inline uint16_t rol16(uint16_t v, unsigned n)
{
    n &= 15;
    return n ? (uint16_t)((v << n) | (v >> (16 - n))) : v;
}

#include "pre2_sym.h"
#endif
