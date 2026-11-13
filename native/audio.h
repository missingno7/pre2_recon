/* Faithful mono audio: commands in, timestamped 168-byte U8 blocks out.
 * No device model and no access to the translated game's memory. */
#ifndef NATIVE_AUDIO_H
#define NATIVE_AUDIO_H
#include <stdint.h>
#include "../c/audio_events.h"
#define AUDIO_BLOCK_BYTES 168u
#define AUDIO_BLOCK_NS (168ull * 119000ull)
typedef struct Audio Audio;
typedef void (*AudioPcm)(void *user, uint64_t start_ns, const uint8_t *pcm);
/* Assets are decoded through assetfs/sqz. Failure is reported, never silent fallback. */
Audio *audio_create(AudioPcm output, void *user);
void audio_destroy(Audio *audio);
int audio_command(Audio *audio, const AudioCmd *cmd, uint64_t now_ns);
uint64_t audio_deadline(const Audio *audio);
/* Called at a logical service point; deadline is the nominal block boundary even
 * when delivery is late. Commands and completions follow the machine's event order. */
void audio_complete(Audio *audio);
#endif
