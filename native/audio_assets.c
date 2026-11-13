/* Pristine decoded assets are retained independently of destructive playback images. */
#include "audio_internal.h"
#include "assetfs.h"
#include "sqz.h"
#include <stdlib.h>
#include <string.h>
static const char *const names[18] = {
    "PRES.TRK", "CODE.TRK", "CARTE.TRK", "PRESENTA.TRK", "GLACE.TRK",
    0,0,0,0,"MINES.TRK","MYSTERY.TRK",0,0,"MONSTER.TRK","FINAL.TRK",
    "BRAVO.TRK","KOOL.TRK","BOULA.TRK"
};
static int load(AudioAsset *s, const char *name) {
    return asset_read(name,&s->packed,&s->packed_n) ||
           sqz_decode(s->packed,s->packed_n,&s->raw,&s->n);
}
int audio_assets_load(Audio *a) {
    if (load(&a->bank,"SAMPLE.SQZ")) return 1;
    for (unsigned i=0;i<18;i++) if (names[i] && load(&a->songs[i],names[i])) return 1;
    /* Convert the whole paragraph allocation, including the sixteen padding bytes.
     * DEVICE subsequently supplies the actual converted loader padding. */
    size_t n=(a->bank.n/16+1)*16;
    if (n>sizeof a->sample) return 1;
    memcpy(a->sample,a->bank.raw,a->bank.n);
    for (size_t i=0;i<n;i++) a->sample[i]=(uint8_t)((uint8_t)(a->sample[i]+0x80)>>2);
    return 0;
}
void audio_assets_free(Audio *a) {
    free(a->bank.raw); free(a->bank.packed);
    for (unsigned i=0;i<18;i++) { free(a->songs[i].raw); free(a->songs[i].packed); }
}
void audio_assets_song(Audio *a, const AudioCmd *cmd) {
    const AudioAsset *s=&a->songs[cmd->id];
    /* PageInResource copies exactly 105B0h bytes, including decoder workspace
     * beyond the decoded allocation. LZSS input starts at the next paragraph.
     * Bytes beyond the copied window retain the previous playback image. */
    uint8_t page[0x105B0]={0};
    size_t n=s->n < sizeof page ? s->n : sizeof page;
    memcpy(page,s->raw,n);
    size_t work=(s->n/16+1)*16;
    if (work<sizeof page && s->packed_n>17) {
        n=s->packed_n-17;
        if (n>sizeof page-work) n=sizeof page-work;
        memcpy(page+work,s->packed+17,n);
    }
    memcpy(a->module,page,sizeof page);
    /* Quirk: inclusive sample limits expose stale allocation padding. Its value
     * depends on earlier non-audio loads, so SONG_END supplies it explicitly. */
    if (cmd->tail_offset && cmd->tail_offset+16<=sizeof a->module)
        memcpy(a->module+cmd->tail_offset,cmd->tail,16);
    /* Instrument headers (MOD sample records, big-endian words, lengths in words). */
    uint16_t mseg = 0; /* private module-relative paragraphs */
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
}
