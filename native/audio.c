/* Command-driven block pipeline. The runtime owns when completions are delivered;
 * this engine owns every byte of playback state and publishes immutable-at-call PCM. */
#include "audio_internal.h"
#include <stdlib.h>
#include <string.h>
extern const uint8_t pre2_image[]; /* immutable linked ASM data, never the live game image */
static const uint16_t periods[35]={
    0x358,0x328,0x2fa,0x2d0,0x2a6,0x280,0x25c,0x23a,0x21a,0x1fc,0x1e0,0x1c5,
    0x1ac,0x194,0x17d,0x168,0x153,0x140,0x12e,0x11d,0x10d,0xfe,0xf0,0xe2,
    0xd6,0xca,0xbe,0xb4,0xaa,0xa0,0x97,0x8f,0x87,0x7f,0x78
};
static const uint16_t steps[37]={
    0x71,0xff7f,0xff87,0xff8f,0xff98,0xffa1,0xffab,0xffb5,0xffc0,0xffcb,
    0xffd7,0xffe4,0xfff1,0,0xf,0x1f,0x30,0x43,0x56,0x6a,0x80,0x97,0xaf,
    0xc8,0xe4,0x100,0x11e,0x140,0x160,0x184,0x1ac,0x1d5,0x1fe,0x22b,0x25e,0x291,0x2c9
};
static const uint16_t sfx_lengths[11]={
    0x188e,0x1c80,0x235e,0x19e6,0xab2,0x912,0,0x35d2,0x6c4,0x1c86,0xe2e
};
Audio *audio_create(AudioPcm output, void *user) {
    Audio *a=calloc(1,sizeof *a);
    if (!a) return NULL;
    a->deadline=UINT64_MAX; a->output=output; a->user=user;
    if (audio_assets_load(a)) { audio_destroy(a); return NULL; }
    SET_DW(DS_PlayBuffer,0x10c9); SET_DW(DS_MixBuffer,0x1171);
    SET_DW(DS_SfxPlayPos,0xffff);
    SET_DB(DS_SongSpeed,1); SET_DB(DS_SongTickCount,1);
    for (unsigned i=0;i<35;i++) SET_DW(DS_PeriodTable+i*2,periods[i]);
    for (unsigned i=0;i<37;i++) SET_DW(DS_StepTable+i*2,steps[i]);
    for (unsigned i=0;i<16;i++) SET_DW(DS_EffectJumpTable+i*2,CS_IgnoreEffect);
    SET_DW(DS_EffectJumpTable+20,CS_EffectVolumeSlide);
    SET_DW(DS_EffectJumpTable+22,CS_EffectPositionJump);
    SET_DW(DS_EffectJumpTable+24,CS_EffectSetVolume);
    SET_DW(DS_EffectJumpTable+26,CS_EffectPatternBreak);
    SET_DW(DS_EffectJumpTable+30,CS_EffectSetSpeed);
    for (unsigned v=0;v<=64;v++) for (unsigned s=0;s<64;s++)
        SET_DB(DS_VolumeTable+v*64+s,v*s/64);
    /* BOULA's B80 jumps beyond the 128-byte order table into the initial IRQ
     * vector table. Preserve those adjacent bytes for the unchecked order read. */
    memcpy(a->data+DS_IrqVectorTable,pre2_image+0x9ff0+DS_IrqVectorTable,32);
    /* Quirk: an inclusive-end read can yield a raw byte >=64. At volume 64,
     * XLAT reads past the LUT into the initial character table / protection text.
     * Keep those immutable neighbour bytes too; they are not clampable silence. */
    memcpy(a->data+0x2301,pre2_image+0x9ff0+0x2301,0xc0);
    uint16_t start=0;
    for (unsigned i=0;i<11;i++) {
        SET_DW(DS_word_1009+i*4,start); SET_DW(DS_word_100B+i*4,sfx_lengths[i]);
        start=(uint16_t)(start+sfx_lengths[i]);
    }
    return a;
}
void audio_destroy(Audio *a) { if (a) { audio_assets_free(a); free(a); } }
static void publish(Audio *a, uint64_t now) {
    if (a->output) a->output(a->user,now,a->data+DW(DS_PlayBuffer));
    a->deadline=now+AUDIO_BLOCK_NS;
}
int audio_command(Audio *a, const AudioCmd *cmd, uint64_t now) {
    switch (cmd->kind) {
    case AUDIO_DEVICE:
        a->device=1; a->enabled=1; a->music=cmd->id!=0;
        if (cmd->tail_offset && cmd->tail_offset+16<=sizeof a->sample)
            memcpy(a->sample+cmd->tail_offset,cmd->tail,16);
        break;
    case AUDIO_SFX:
        if (a->device) {
            uint16_t i=(uint16_t)(cmd->id<<2);
            SET_DW(DS_SfxPlayPos,DW((uint16_t)(DS_word_1009+i)));
            SET_DW(DS_SfxBytesLeft,DW((uint16_t)(DS_word_100B+i)));
        }
        break;
    case AUDIO_SONG_BEGIN: a->ready=0; break;
    case AUDIO_SONG_END:
        if (cmd->id>=18 || !a->songs[cmd->id].raw) return 1;
        audio_assets_song(a,cmd); break;
    case AUDIO_OUTPUT_READY: a->ready=cmd->id!=0; break;
    case AUDIO_OUTPUT_TOGGLE: a->enabled=cmd->id!=0; break;
    case AUDIO_OUTPUT_START: if (a->device) publish(a,now); break;
    default: return 1;
    }
    return 0;
}
uint64_t audio_deadline(const Audio *a) { return a->deadline; }
void audio_complete(Audio *a) {
    uint64_t t=a->deadline;
    if (t==UINT64_MAX) return;
    a->deadline=UINT64_MAX;
    if (!a->device || !a->ready || !a->enabled) return;
    uint16_t play=DW(DS_PlayBuffer);
    SET_DW(DS_PlayBuffer,DW(DS_MixBuffer)); SET_DW(DS_MixBuffer,play);
    /* Start the prepared block before touching the other buffer. Initial zeros and
     * stale buffers after a restart are intentional (audit section 6). */
    publish(a,t);
    if (a->music) audio_tracker(a);
    audio_mix(a);
}
