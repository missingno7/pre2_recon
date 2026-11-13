#ifndef AUDIO_INTERNAL_H
#define AUDIO_INTERNAL_H
#include "audio.h"
#include "../c/pre2_sym.h"
#include <stddef.h>
/* Compatibility arena, deliberately laid out like the audio portion of DATA.
 * A typed four-voice struct cannot represent the original wrong-index ChannelEnd
 * store, or mixing past byte 167 into the next buffer (audit sections 4 and 6).
 * The 16-bit accessors preserve wraps and unsigned XLAT reads. This is private
 * audio storage, never an alias or snapshot of pre2_mem. Far sample segments are
 * relative paragraphs in our module image, not DOS addresses. The larger backing
 * covers all 256 unchecked pattern indices plus a wrapped 64 KiB row offset;
 * beyond the owned loader window it starts at zero (the bounded boot arena).
 * In particular BOULA's B80 order reads the IRQ-table FFh byte as pattern 255. */
typedef struct AudioAsset { uint8_t *raw, *packed; size_t n, packed_n; } AudioAsset;
struct Audio {
    uint8_t data[65536], module[0x50000], sample[65536];
    AudioAsset bank, songs[18];
    unsigned enabled, ready, device, music;
    uint64_t deadline;
    AudioPcm output; void *user;
};
static inline uint8_t ab(const Audio *a, uint16_t off) { return a->data[off]; }
static inline uint16_t aw(const Audio *a, uint16_t off) {
    return (uint16_t)(ab(a,off) | ab(a,(uint16_t)(off+1)) << 8);
}
static inline void asw(Audio *a, uint16_t off, uint16_t v) {
    a->data[off]=(uint8_t)v; a->data[(uint16_t)(off+1)]=(uint8_t)(v>>8);
}
static inline uint8_t am8(const Audio *a, uint16_t seg, uint16_t off) {
    size_t p=(size_t)seg*16+off;
    return p < sizeof a->module ? a->module[p] : 0;
}
static inline uint16_t am16(const Audio *a, uint16_t seg, uint16_t off) {
    return (uint16_t)(am8(a,seg,off) | am8(a,seg,(uint16_t)(off+1)) << 8);
}
static inline void amset(Audio *a, uint16_t seg, uint16_t off, uint8_t v) {
    size_t p=(size_t)seg*16+off;
    if (p < sizeof a->module) a->module[p]=v;
}
static inline void amset16(Audio *a, uint16_t seg, uint16_t off, uint16_t v) {
    amset(a,seg,off,(uint8_t)v); amset(a,seg,(uint16_t)(off+1),(uint8_t)(v>>8));
}
#define DB(o) ab(a,(uint16_t)(o))
#define DW(o) aw(a,(uint16_t)(o))
#define SET_DB(o,v) (a->data[(uint16_t)(o)]=(uint8_t)(v))
#define SET_DW(o,v) asw(a,(uint16_t)(o),(uint16_t)(v))
#define rd8(s,o) am8(a,(s),(o))
#define rd16(s,o) am16(a,(s),(o))
#define wr8(s,o,v) amset(a,(s),(o),(v))
#define wr16(s,o,v) amset16(a,(s),(o),(v))
void audio_tracker(Audio *a);
void audio_mix(Audio *a);
int audio_assets_load(Audio *a);
void audio_assets_free(Audio *a);
void audio_assets_song(Audio *a, const AudioCmd *cmd);
#endif
