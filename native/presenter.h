/* Pure presentation of immutable published frames. No machine or SDL dependency.
 * The caller serializes push/render/reset. Assets are borrowed for the session;
 * destroy/reset BEFORE compositor_init, or retain the assets across that reset.
 * See docs/native-presenter.md for the time and endpoint conventions. */
#ifndef PRE2_PRESENTER_H
#define PRE2_PRESENTER_H
#include "enh_render.h"

#define PRESENTER_MAX_W 2048
#define PRESENTER_MAX_H 480
/* Three original PIT ticks (divisor 4000h), not three CRT starts. */
#define PRESENTER_DELAY_NS (49152ull * 1000000000ull / 1193182ull)
/* Identity is authoritative. Only a matched instance jumping >256 world pixels
 * in one view interval is treated as a teleport (Euclidean distance). */
#define PRESENTER_TELEPORT_PX 256.0

typedef enum PresenterMode { PRESENT_FAITHFUL, PRESENT_ENHANCED, PRESENT_WIDESCREEN } PresenterMode;
typedef struct PresenterOptions {
    PresenterMode mode;
    int interpolate, smooth_camera, smooth_transitions;
    double camera_tau; /* seconds, >0: larger = softer; default .12, both axes */
    double aspect;     /* width/height, 0 = caller's window aspect */
    EnhBackdrop backdrop;
    EnhHud hud;
} PresenterOptions;
typedef struct PresenterPose {
    const FrameGameView *view; /* newer discrete set in the open interval */
    EnhPosition positions[FRAME_MAX_SPRITES];
    size_t count;
    double x, y, alpha;
} PresenterPose;
typedef struct PresenterCamera {
    double x, y;
    uint64_t time;
    uint32_t epoch, cut_frame, player_id;
    int seeded;
} PresenterCamera;
typedef struct PresenterImage {
    uint32_t *pixels;
    size_t capacity; /* pixels, tightly packed output */
    int w, h;
    uint64_t time; /* sampled logical time, <= bound */
    double camera_x, camera_y, alpha, progress, radius;
    unsigned fallback; /* bit 1: missing world; 2: iris data; 4: map layers; 8: faithful pixels in non-faithful mode */
} PresenterImage;
typedef struct Presenter Presenter;
PresenterOptions presenter_defaults(void);
Presenter *presenter_create(const PresenterOptions *options);
void presenter_destroy(Presenter *p);
void presenter_reset(Presenter *p);
/* Copies the publication, including its image. Non-increasing times are ignored. */
void presenter_push(Presenter *p, const Pre2Frame *frame);
/* Rendering never publishes, steps, polls input or produces audio. Window aspect
 * is read only when options.aspect==0. Returns -1 for no frame/invalid buffer. */
int presenter_render(Presenter *p, uint64_t bound, double window_aspect, PresenterImage *out);

/* Factored deterministic motion/camera operations, also useful to test consumers. */
void presenter_interpolate(const FrameGameView *a, const FrameGameView *b, double alpha, PresenterPose *out);
int presenter_fixed_aperture(const FrameGameView *v, int width);
void presenter_camera(PresenterCamera *state, const PresenterPose *pose,
                      uint64_t time, int width, double tau, double *x, double *y);
#endif
