/* Product preferences, independent of SDL and the game's own configuration. */
#ifndef PRE2_SETTINGS_H
#define PRE2_SETTINGS_H
#include <stdio.h>
#include "presenter.h"

typedef struct Pre2Settings {
    int mode, aspect, interpolate, smooth_camera, smooth_transitions;
    double camera_tau;
    int hud, backdrop, wide_sim; /* -1 auto, 0 off, otherwise 320..640 by 16 */
    int fullscreen, window_scale, vsync, frame_cap, integer_scaling, pixel_aspect;
    int volume, mute, performance;
} Pre2Settings;
typedef enum Pre2Setting {
    SET_MODE, SET_ASPECT, SET_INTERPOLATE, SET_SMOOTH_CAMERA, SET_SMOOTH_TRANSITIONS,
    SET_CAMERA_TAU, SET_HUD, SET_BACKDROP, SET_WIDE_SIM, SET_FULLSCREEN,
    SET_WINDOW_SCALE, SET_VSYNC, SET_FRAME_CAP, SET_INTEGER_SCALING,
    SET_PIXEL_ASPECT, SET_VOLUME, SET_MUTE, SET_PERFORMANCE, SET_COUNT
} Pre2Setting;
Pre2Settings settings_defaults(void);
const char *settings_key(Pre2Setting id);
/* Scalar text accepts enum names, numbers, true/false (also on/off for CLI). */
int settings_set(Pre2Settings *s, Pre2Setting id, const char *value);
void settings_value(const Pre2Settings *s, Pre2Setting id, char *out, size_t n);
void settings_copy(Pre2Settings *dst, const Pre2Settings *src, Pre2Setting id);
void settings_adjust(Pre2Settings *s, Pre2Setting id, int direction);
PresenterOptions settings_presenter(const Pre2Settings *s);
/* A malformed document leaves defaults. Invalid known values use that key's
 * default; valid siblings survive. Warnings go to warnings, if non-NULL.
 * Missing files return 1, other errors -1, success 0. */
int settings_parse(Pre2Settings *out, const char *json, FILE *warnings);
int settings_load(Pre2Settings *out, const char *path, FILE *warnings);
int settings_save(const Pre2Settings *s, const char *path, FILE *warnings);
#endif
