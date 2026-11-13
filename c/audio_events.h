/* Semantic audio commands, emitted at their origins in the translated game (docs/native.md,
 * "Audio"; evidence: build/audit/audio_native.md).
 *
 * The game's own sound state (the SFX voice, the tracker, the mixer buffers in DATA) keeps
 * being written exactly as the original writes it; in addition each origin reports what it
 * means, in program order, to a sink the native runtime installs. The native audio engine
 * consumes only these commands (stamped with logical time by the runtime) and owns its
 * playback state; it never reads or writes game memory. No sink in the differential tests.
 *
 * Every trigger is reported, including identical repeats in one frame: the original has one
 * replaceable SFX voice and the last trigger before a mix wins, so the engine needs them all.
 */
#ifndef AUDIO_EVENTS_H
#define AUDIO_EVENTS_H
#include <stdint.h>

typedef enum AudioCmdKind {
    AUDIO_SFX = 1,          /* StartSoundEffect: id = effect number (the whole DX, as the original indexes) */
    AUDIO_SONG_BEGIN,       /* ActivateResourceSlot, real activation: id = slot; output readiness cleared */
    AUDIO_SONG_END,         /* the module is rebuilt and the tracker reset (order 0, row 0, speed 6) */
    AUDIO_OUTPUT_START,     /* StartSbBlock: the block chain (re)starts from the current play buffer */
    AUDIO_OUTPUT_TOGGLE,    /* F3: SoundState bit 0 toggled; id = the new bit 0 */
    AUDIO_DEVICE,           /* InitSoundDevices on the Sound Blaster: id = 1 when the music path is enabled */
    /* The /B sound-update tail sets readiness even without a module rebuild. Kept
     * separate from START: START publishes the stale buffer before readiness is set. */
    AUDIO_OUTPUT_READY,     /* resource_sound_update: id = new readiness bit */
} AudioCmdKind;

/* Where a sound came from (for later stereo placement; the faithful mono mix ignores it). */
typedef enum AudioSourceKind {
    AUDIO_SRC_NONE = 0,     /* no position at the origin (e.g. the level-9 script) */
    AUDIO_SRC_PLAYER,       /* the player record (ds:4F1Ch) */
    AUDIO_SRC_RECORD,       /* a render/object/pool record: ref = its DS offset */
    AUDIO_SRC_BOSS,         /* a boss part / body record: ref = its DS offset */
} AudioSourceKind;
/* OR into audio_source's kind for the exit tally's screen-local flight records. */
#define AUDIO_SOURCE_SCREEN 0x80u


typedef struct AudioCmd {
    uint16_t kind;          /* AudioCmdKind */
    uint16_t id;
    uint8_t  src_kind;      /* AudioSourceKind */
    uint16_t src_ref;       /* DS offset of the source record, 0 if none */
    int16_t  x, y;          /* the source's position (pixels) at the trigger, when src_kind != NONE */
    uint32_t game_frame;    /* pre2_frames_run at the trigger (diagnostics) */
    uint8_t src_space;      /* 0 world pixels; 1 screen-local pixels (exit tally) */
    /* SONG_END carries the sixteen bytes following the decoded samples. The
     * audit's inclusive limit reads one byte beyond the final instrument; that
     * byte is stale loader allocation padding, not part of the disk asset.
     * DEVICE carries the converted SAMPLE allocation padding at its bank end.
     * Capture at the actual loader boundary, never infer from playback memory.
     * tail_offset==0 means no tail payload (for synthetic clients). */
    uint32_t tail_offset;
    uint8_t tail[16];
} AudioCmd;

/* Installed by the runtime; unset in the differential tests. */
extern void (*pre2_audio_sink)(const AudioCmd *cmd);

/* The next StartSoundEffect's source (set by the caller just before it; consumed once). */
void audio_source(uint8_t kind, uint16_t ref, uint16_t x, uint16_t y);
void audio_emit(uint16_t kind, uint16_t id);
void audio_emit_tail(uint16_t kind, uint16_t id, uint32_t tail_offset, const uint8_t tail[16]);
#endif
