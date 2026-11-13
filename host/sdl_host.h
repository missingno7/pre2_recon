/* SDL3 services of the native host: a window that shows an XRGB frame, an audio stream fed
 * with the PCM the game produced (SDL resamples it to the device: the output stage), and the
 * keyboard as PC scan codes. Nothing here runs game code or decides game timing. */
#ifndef SDL_HOST_H
#define SDL_HOST_H
#include <stdint.h>
int  sdl_host_init(int video, int sound);
void sdl_host_shutdown(void);
/* Pumps events; each key becomes PC scan codes (E0h prefix for the extended group). Returns 1
 * when the window was closed. */
int  sdl_host_pump(void (*key)(uint8_t code));
/* Shows a w x h frame letterboxed at the aspect_w x aspect_h logical size. */
void sdl_host_show(const uint32_t *xrgb, int w, int h, int aspect_w, int aspect_h);
void sdl_host_audio(const uint8_t *pcm, uint32_t n, uint32_t rate);
uint32_t sdl_host_audio_queued(void);
uint64_t sdl_host_ns(void);
void sdl_host_sleep_ns(uint64_t ns);
uint8_t sdl_pc_scancode(int sdl_scancode, int *extended);
#endif
