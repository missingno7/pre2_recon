/* Exact integer mixer, independently owned state; oracle: c/sound.c and
 * src/MODULE1.ASM MixChannel/SbMixBlock. See audio_internal.h for aliasing. */
#include "audio_internal.h"

#define VOLUME_TABLE DS_VolumeTable   /* 65 rows of 64 bytes, filled at SB set-up (loc_1E90) */

/* ds offset of a channel field, and of a field of the instrument record at `rec`. */
#define CH_OFF(name, ch)       ((uint16_t)(DS_##name + (ch)))
#define CHW(name, ch)          DW(CH_OFF(name, ch))
#define SET_CHW(name, ch, v)   SET_DW(CH_OFF(name, ch), (v))
#define REC_W(field, rec)      DW((uint16_t)(DS_##field + (rec)))

/* MixChannelOneShotFade: a one-shot sample ended inside the block. AX = P (the previous
 * output byte, looplen high byte is zero); P is added to the following bytes as P, P-1,
 * ..., 1 while CX lasts. The fade does not touch the channel's fields. */
static void one_shot_fade(Audio *a, uint16_t *di, uint16_t *cx)
{
    uint16_t decay = DB((uint16_t)(*di - 1));
    if (decay == 0) return;
    do {
        SET_DB(*di, DB(*di) + (uint8_t)decay);
        *di = (uint16_t)(*di + 1);
        decay--;
        if (decay == 0) return;
    } while (--*cx != 0);
}

static void mix_channel(Audio *a, uint16_t ch)
{
    SET_DW(DS_MixChannelOffset, ch);
    uint16_t vol_row = (uint16_t)(CHW(ChannelVolume, ch) << 6);  /* volume*64, 16-bit */
    uint16_t step = CHW(ChannelStep, ch);
    uint16_t pos = CHW(ChannelPos, ch);
    if (pos == 0xFFFFu) return;                                /* silent (loc_227B) */

    uint16_t rec = (uint16_t)(CHW(ChannelInstrument, ch) << 4);
    uint16_t sample_seg = REC_W(word_0BDC, rec);
    uint16_t si = (uint16_t)(REC_W(word_0BDA, rec) + pos);
    uint16_t cx = (uint16_t)(CHW(ChannelEnd, ch) - pos);
    uint16_t phase = CHW(ChannelPhase, ch);
    uint16_t vrow = (uint16_t)(vol_row + VOLUME_TABLE);
    /* the original keeps the limit in SP: SP = sample offset + ChannelEnd (16-bit) */
    uint16_t limit = (uint16_t)(si + cx);
    /* MixSavedSp is CPU stack scratch; no PCM consumer (audit section 6). */
    uint16_t di = DW(DS_MixBuffer);
    uint16_t count = AUDIO_BLOCK_BYTES;

    for (;;) {                                                 /* loc_21E6 */
        uint8_t s = rd8(sample_seg, si);
        si = (uint16_t)(si + 1);
        uint8_t v = DB((uint16_t)(vrow + s));                  /* xlat: DS:[BX+AL] */
        SET_DB(di, DB(di) + v);                                /* 8-bit add, carry dropped */
        di = (uint16_t)(di + 1);
        phase = (uint16_t)(phase + step);
        int16_t adv = (int8_t)(phase >> 8);                 /* cbw of DH */
        phase &= 0x00FFu;                                   /* xor dh,dh: keep the fraction */
        si = (uint16_t)(si + (uint16_t)adv);
        if (si > limit) {                                      /* MixChannelWrap */
            SET_DW(DS_MixSavedVolumeRow, vrow);
            uint16_t looplen = REC_W(word_0BD8, rec);
            if (looplen <= 2) {                                /* MixChannelOneShotFade */
                one_shot_fade(a, &di, &count);
                break;
            }
            si = REC_W(word_0BD6, rec);                        /* raw loop start (quirk) */
            /* stray store: ChannelEnd array indexed by the instrument's record offset */
            SET_DW((uint16_t)(DS_ChannelEnd + rec), (uint16_t)(looplen + si));
            continue;                                          /* jmp loc_21E6: CX unchanged */
        }
        if (--count == 0) break;
    }

    /* MixChannelStore */
    SET_CHW(ChannelPhase, ch, phase);
    si = (uint16_t)(si - REC_W(word_0BDA, rec));
    if (CHW(ChannelEnd, ch) > si) {                            /* loc_2277 */
        SET_CHW(ChannelPos, ch, si);
        return;
    }
    si = 0xFFFFu;
    uint16_t looplen = REC_W(word_0BD8, rec);
    if (looplen > 2) {
        si = REC_W(word_0BD6, rec);
        SET_CHW(ChannelEnd, ch, (uint16_t)(looplen + si));
    }
    SET_CHW(ChannelPos, ch, si);
}

void audio_mix(Audio *a)
{
    uint16_t di = DW(DS_MixBuffer);
    uint16_t left = DW(DS_SfxBytesLeft);
    if (left == 0) {                                           /* SfxSilenceBlock */
        for (uint16_t k = 0; k < AUDIO_BLOCK_BYTES; k++) SET_DB((uint16_t)(di + k), 0);
    } else {
        uint16_t n = left < AUDIO_BLOCK_BYTES ? left : AUDIO_BLOCK_BYTES;
        SET_DW(DS_SfxBytesLeft, (uint16_t)(left - n));
        uint16_t si = DW(DS_SfxPlayPos);

        /* rep movsw: n/2 words; an odd n drops its last byte and leaves the block bytes
         * A6h-A7h unwritten (quirk). Offsets wrap within the segments. */
        uint16_t copy = (uint16_t)((n >> 1) * 2);
        for (uint16_t k = 0; k < copy; k++)
            SET_DB((uint16_t)(di + k), a->sample[(uint16_t)(si + k)]);
        si = (uint16_t)(si + copy);
        di = (uint16_t)(di + copy);
        uint16_t zero = (uint16_t)(((AUDIO_BLOCK_BYTES - n) >> 1) * 2);  /* rep stosw */
        for (uint16_t k = 0; k < zero; k++) SET_DB((uint16_t)(di + k), 0);
        SET_DW(DS_SfxPlayPos, si);
    }
    if (!a->music) return;                     /* test cs:OptionBits, 40h */
    mix_channel(a, 0);
    mix_channel(a, 2);
    mix_channel(a, 4);
    if (DW(DS_SfxBytesLeft) != 0) return;
    mix_channel(a, 6);
}

