/* No faithful pixels in the enhanced/widescreen presentation: every publication of the
 * scripted scenarios is rendered by enhanced and widescreen presenters (with and without
 * smoothing) right after it is pushed, and the presenter's fallback bit 8 ("faithful pixels
 * in a non-faithful mode") is counted per screen family. Gameplay, gameplay transitions and
 * every screen with a valid native screen view must be native. Publications that carry
 * neither a world view nor a screen view are reported separately (they have nothing native
 * to draw from); the target is none. Unsmoothed enhanced output at 320 equals faithful for
 * every publication (also checked by presenter_purity). */
#include "script.h"
#include "c/vga.h"
#include "native/compositor.h"
#include "native/presenter.h"

enum { MODES = 4 };
static Presenter *present[MODES];
static const char *mode_names[MODES] = {"enhanced", "wide 16:9", "wide 21:9 smooth", "enhanced smooth"};
static Pre2Frame frame;
static uint32_t pixels[PRESENTER_MAX_W * PRESENTER_MAX_H], expected[FRAME_MAX_W * FRAME_MAX_H];
static unsigned faithful_px[MODES][3], renders[MODES][3], neither, parity_checks;
static const char *neither_phase;
static const char *family[3] = {"gameplay", "screen", "other"};

static void display(uint64_t t, uint16_t start)
{
    compositor_publish(t, start, &frame);
    unsigned fam = frame.game.valid && (frame.game.world_valid || frame.transition.kind) ? 0 :
                   frame.screen_view.valid ? 1 : 2;
    if (fam == 2) { neither++; if (!neither_phase) neither_phase = frame.screen; }
    frame_to_xrgb(&frame, expected);
    for (unsigned i = 0; i < MODES; i++) {
        presenter_push(present[i], &frame);
        PresenterImage im = {0}; im.pixels = pixels; im.capacity = sizeof pixels / 4;
        if (presenter_render(present[i], t, 16.0 / 9, &im)) continue;
        renders[i][fam]++;
        if (im.fallback & 8) faithful_px[i][fam]++;
        if (i == 0 && im.w == frame.w && im.h == frame.h) {
            if (memcmp(pixels, expected, (size_t)im.w * im.h * 4)) {
                fprintf(stderr, "FAIL enhanced 320 differs from faithful at %.6f (%s, family %s)\n",
                        t / 1e9, frame.screen ? frame.screen : "?", family[fam]);
                exit(1);
            }
            parity_checks++;
        }
    }
}

static void run(const char *assets, const char *scenario, double seconds, unsigned year)
{
    pre2_frames_run = 0; script_reset(scenario);
    rt_boot(assets, year, 6, 1); vga_memory_off = 1; compositor_init();
    for (unsigned i = 0; i < MODES; i++) {
        PresenterOptions o = presenter_defaults();
        o.mode = i == 0 || i == 3 ? PRESENT_ENHANCED : PRESENT_WIDESCREEN;
        o.aspect = i == 2 ? 21.0 / 9 : 16.0 / 9;
        if (i >= 2) o.interpolate = o.smooth_transitions = 1;
        present[i] = presenter_create(&o);
        if (!present[i]) exit(1);
    }
    RtConfig cfg = {0}; cfg.audio_native = 1;
    rt_on_service = script_service; rt_on_display_frame = display; rt_on_pcm = 0;
    if (rt_init(&cfg)) exit(1);
    if (rt_run_until((uint64_t)(seconds * 1e9)) != RT_STEP_DONE) exit(1);
    for (unsigned i = 0; i < MODES; i++) { presenter_destroy(present[i]); present[i] = 0; }
}

int main(int argc, char **argv)
{
    const char *assets = arg_assets(argc, argv);
    static const struct { const char *name; double seconds; unsigned year; } cases[] = {
        {"attract", 90, 1995}, {"warp", 45, 1995}, {"ending", 70, 1995},
        {"gameover", 60, 1995}, {"castle", 70, 1995},
    };
    for (unsigned k = 0; k < sizeof cases / sizeof cases[0]; k++) run(assets, cases[k].name, cases[k].seconds, cases[k].year);
    int fail = 0;
    for (unsigned i = 0; i < MODES; i++) {
        printf("   %-17s", mode_names[i]);
        for (unsigned f = 0; f < 3; f++) printf("  %s %u/%u faithful", family[f], faithful_px[i][f], renders[i][f]);
        printf("\n");
        if (faithful_px[i][0] || faithful_px[i][1]) fail = 1;
    }
    printf("   publications with neither a world nor a screen view: %u%s%s\n", neither,
           neither_phase ? " (first in " : "", neither_phase ? neither_phase : "");
    if (neither_phase) printf(")\n");
    printf("   enhanced 320 equals faithful at %u publications\n", parity_checks);
    if (fail) { fprintf(stderr, "FAIL faithful pixels in gameplay or screen presentation\n"); return 1; }
    printf("presenter_native: PASS\n");
    return 0;
}
