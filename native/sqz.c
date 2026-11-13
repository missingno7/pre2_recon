#include "sqz.h"
#include <stdlib.h>
#include <string.h>

typedef struct { const uint8_t *in; size_t n, p; uint8_t *out; size_t len, cap; int bad; } State;
static unsigned byte(State *s) { if (s->p >= s->n) { s->bad = 1; return 0; } return s->in[s->p++]; }
static unsigned word(const uint8_t *p) { return p[0] | (unsigned)p[1] << 8; }
static void put(State *s, unsigned c) {
    if (s->bad) return;
    if (s->len == s->cap) { size_t cap = s->cap ? s->cap * 2 : 4096; uint8_t *p;
        if (cap > 1048576 || !(p = realloc(s->out, cap))) { s->bad = 1; return; }
        s->out = p; s->cap = cap;
    }
    s->out[s->len++] = (uint8_t)c;
}
typedef struct { State *s; unsigned bits, left; } Lz;
static int lzbit(Lz *b) {
    int v = b->bits & 1; b->bits >>= 1;
    if (!--b->left) { unsigned lo = byte(b->s), hi = byte(b->s); b->bits = lo | hi << 8; b->left = 16; }
    return v;
}
static void lz(State *s) {
    Lz b = { s, 0, 16 }; unsigned lo, hi;
    s->p = 17; lo = byte(s); hi = byte(s); b.bits = lo | hi << 8;
    while (!s->bad) {
        unsigned bl, bh = 255, count;
        if (lzbit(&b)) { put(s, byte(s)); continue; }
        int form = lzbit(&b); bl = byte(s);
        if (!form) {
            if (!lzbit(&b)) { if (bl == 255) return; count = 2; }
            else { for (int i = 0; i < 3; i++) bh = (bh << 1 | lzbit(&b)) & 255; bh = (bh - 1) & 255; count = 2; }
        } else {
            bh = (bh << 1 | lzbit(&b)) & 255;
            if (!lzbit(&b)) { unsigned dh = 2; for (int i = 0; i < 3; i++) { if (lzbit(&b)) break; bh = (bh << 1 | lzbit(&b)) & 255; dh *= 2; } bh = (bh - dh) & 255; }
            unsigned dh = 2; int done = 0;
            for (int i = 0; i < 4 && !done; i++) { dh++; if (lzbit(&b)) done = 1; }
            if (done) count = dh;
            else if (lzbit(&b)) { dh++; count = dh + lzbit(&b); }
            else if (lzbit(&b)) count = byte(s) + 17;
            else { unsigned v = 0; for (int i = 0; i < 3; i++) v = v << 1 | lzbit(&b); count = v + 9; }
        }
        unsigned dist = 65536 - (bh << 8 | bl);
        if (dist > s->len) { s->bad = 1; break; }
        while (count-- && !s->bad) put(s, s->out[s->len - dist]);
    }
}
static unsigned code(State *s, size_t *bit, unsigned width) {
    unsigned v = 0; for (unsigned i = 0; i < width; i++, (*bit)++) {
        if (*bit / 8 >= s->n) { s->bad = 1; return 0; }
        v = v << 1 | ((s->in[*bit / 8] >> (7 - *bit % 8)) & 1);
    } return v;
}
static void lzw(State *s) {
    uint16_t prefix[4096] = {0}; uint8_t suffix[4096] = {0}, stack[4097]; size_t bit = 32;
    unsigned width, next, threshold, prev, first;
    for (;;) {
        width = 9; next = 258; threshold = 512;
        prev = code(s, &bit, width); if (s->bad || prev == 257) return;
        first = prev & 255; put(s, first);
        for (;;) {
            if (next == threshold && width != 12) { width++; threshold <<= 1; }
            unsigned cur = code(s, &bit, width), v = cur, sp = 0;
            if (s->bad || cur == 257) return;
            if (cur == 256) break;
            if (next <= v) { stack[sp++] = (uint8_t)first; v = prev; }
            while (v >= 256) { if (v >= next || sp >= 4096) { s->bad = 1; return; } stack[sp++] = suffix[v]; v = prefix[v]; }
            stack[sp++] = (uint8_t)v; first = v;
            while (sp && !s->bad) put(s, stack[--sp]);
            if (next < 4096) { suffix[next] = (uint8_t)first; prefix[next++] = (uint16_t)prev; }
            prev = cur;
        }
    }
}
typedef struct { State *s; size_t tree, tree_n, bit, bytes; } Huff;
static int symbol(Huff *h) {
    size_t node = 0;
    for (unsigned steps = 0; steps < 32768; steps++) {
        if (h->bit >= h->bytes * 8) return -1;
        size_t idx = h->bit++ / 8; unsigned shift = 7 - ((h->bit - 1) % 8);
        size_t actual = h->s->n - h->tree - h->tree_n;
        unsigned v = idx < actual ? h->s->in[h->tree + h->tree_n + idx] :
                     actual >= 0x2800 ? h->s->in[h->tree + h->tree_n + actual - 0x2800] : 0;
        unsigned bit = (v >> shift) & 1;
        size_t at = node + bit * 2;
        if (at + 2 > h->tree_n) { h->s->bad = 1; return -1; }
        unsigned child = word(h->s->in + h->tree + at);
        if (child & 0x8000) return (int)(child & 0x7fff);
        node = child;
    }
    h->s->bad = 1; return -1;
}
static void huff(State *s) {
    size_t tree = word(s->in + 4); if (tree > s->n - 6) { s->bad = 1; return; }
    size_t bytes = s->n - 6 - tree;
    /* An odd final refill decodes a stale byte left by the preceding 2800h-byte refill. */
    Huff h = { s, 6, tree, 0, bytes + (bytes & 1) };
    for (;;) {
        int v = symbol(&h); if (v < 0) return;
        if (v < 256) put(s, (unsigned)v);
        else { unsigned count = v & 255; if (count < 2) { int a = symbol(&h); if (a < 0) return; if (!count) count = (unsigned)a; else { int b = symbol(&h); if (b < 0) return; count = ((unsigned)a & 255) << 8 | ((unsigned)b & 255); } }
            if (!s->len) { s->bad = 1; return; }
            unsigned prev = s->out[s->len - 1]; while (count-- && !s->bad) put(s, prev);
        }
        if (s->bad) return;
    }
}
int sqz_decode(const uint8_t *in, size_t n, uint8_t **out, size_t *out_n) {
    if (!out || !out_n) return 1;
    *out = NULL; *out_n = 0;
    if (!in || n < 6) return 1;
    State s = { in, n, 0, NULL, 0, 0, 0 };
    if (word(in) == 0x4cb4) { if (n < 17 || word(in + 4) != 0x899d || word(in + 6) != 0x6c64) return 1; lz(&s); }
    else if (in[1] == 0x10) lzw(&s); else huff(&s);
    if (s.bad) { free(s.out); return 1; }
    if (!s.out && !(s.out = malloc(1))) return 1;
    *out = s.out; *out_n = s.len; return 0;
}

