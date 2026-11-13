/* Semantic audio commands (see audio_events.h). Presentation output, not game state. */
#include "pre2.h"
#include "audio_events.h"

extern uint32_t pre2_frames_run;
void (*pre2_audio_sink)(const AudioCmd *cmd) = 0;

static AudioCmd pending_src;   /* the source context set by the caller of the next trigger */

void audio_source(uint8_t kind, uint16_t ref, uint16_t x, uint16_t y)
{
    pending_src.src_kind = kind & 0x7Fu; pending_src.src_space = (kind & AUDIO_SOURCE_SCREEN) != 0; pending_src.src_ref = ref;
    pending_src.x = (int16_t)x; pending_src.y = (int16_t)y;
}

void audio_emit(uint16_t kind, uint16_t id)
{
    AudioCmd c = { 0 };
    c.kind = kind; c.id = id; c.game_frame = pre2_frames_run;
    if (kind == AUDIO_SFX) {
        c.src_kind = pending_src.src_kind; c.src_ref = pending_src.src_ref;
        c.src_space = pending_src.src_space;
        c.x = pending_src.x; c.y = pending_src.y;
        pending_src = (AudioCmd){ 0 };
    }
    if (pre2_audio_sink) pre2_audio_sink(&c);
}

/* Loader residue is audio input: the inclusive mixer limit can read it. */
void audio_emit_tail(uint16_t kind, uint16_t id, uint32_t tail_offset, const uint8_t tail[16])
{
    AudioCmd c = { 0 };
    c.kind = kind; c.id = id; c.game_frame = pre2_frames_run;
    c.tail_offset = tail_offset;
    for (unsigned i=0;i<16;i++) c.tail[i]=tail[i];
    if (pre2_audio_sink) pre2_audio_sink(&c);
}
