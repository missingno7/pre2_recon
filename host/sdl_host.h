/* SDL3 services of the native host: a window that shows an XRGB frame, an audio stream fed
 * with the PCM the game produced (SDL resamples it to the device: the output stage), and the
 * keyboard as PC scan codes. Nothing here runs game code or decides game timing. */
#ifndef SDL_HOST_H
#define SDL_HOST_H
#include <stdint.h>
#include "../native/settings.h"
int  sdl_host_init(int video, int sound);
/* The game data directory when none is named: the first of the executable's directory,
 * its assets/ subdirectory, the working directory and ./assets that holds SPRITES.SQZ. */
int  sdl_host_find_assets(char *out, unsigned size);
/* A message box (the product has no console) and stderr. */
void sdl_host_fatal(const char *message);
void sdl_host_shutdown(void);
/* UI sees each PC code before scan-code delivery; return nonzero to consume it.
 * code=0/down=0 queries modality for deferred game-key releases. Returns 1 on
 * window close. No mouse/gamepad input is currently delivered by this host. */
int  sdl_host_pump(void (*key)(uint8_t code), int (*ui)(int code,int down,int repeat,int shift));
/* Product settings; gain is applied to the output stream, never generated PCM. */
void sdl_host_apply(const Pre2Settings *settings);
void sdl_host_audio_pause(int paused);
int sdl_host_settings_path(char *out,unsigned size);
void sdl_host_window_size(int *w,int *h);
/* Shows a w x h frame letterboxed at the aspect_w x aspect_h logical size. */
void sdl_host_show(const uint32_t *xrgb, int w, int h, int aspect_w, int aspect_h);
/* Unsigned 8-bit mono blocks as the machine produced them; see the output stage's notes. */
void sdl_host_audio(const uint8_t *pcm, uint32_t n, uint32_t rate);
uint32_t sdl_host_audio_queued(void);
uint64_t sdl_host_ns(void);
void sdl_host_sleep_ns(uint64_t ns);
uint8_t sdl_pc_scancode(int sdl_scancode, int *extended);
double sdl_host_window_aspect(void);
/* One simulation worker; the bridge lock covers only bounded copies/input, never
 * rendering or vsync. Join it before accessing live machine state/shutdown. */
int sdl_host_worker_start(int (*run)(void *), void *user);
int sdl_host_worker_join(void);
void sdl_host_lock(void);
void sdl_host_unlock(void);
#endif
