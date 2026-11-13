/* The production path publishes the frames the parity reference was checked against.
 *
 * render_parity proves the compositor equal to the VGA model with video memory modelled and
 * the reference Sound Blaster path. Production runs with neither (vga_memory_off, native
 * audio). For each scenario both configurations publish a Pre2Frame at every display start;
 * the sequence of (time, size, blanking, indices, palette, semantic view, transition) must be
 * identical, frame by frame. */
#include "script.h"
#include "c/vga.h"
#include "native/compositor.h"

#define MAXF 20000
static uint64_t hashes[2][MAXF];
static unsigned counts[2];
static int cur;
static Pre2Frame f;

static void on_display(uint64_t t, uint16_t start)
{
    compositor_publish(t, start, &f);
    uint64_t h = fnv64(&t, sizeof t, FNV0);
    h = fnv64(&f.w, sizeof f.w, h); h = fnv64(&f.h, sizeof f.h, h); h = fnv64(&f.blank, 1, h);
    h = fnv64(f.index, (size_t)f.w * f.h, h);
    h = fnv64(f.palette, sizeof f.palette, h);
    h = fnv64(&f.transition, sizeof f.transition, h);
    if (f.game.valid) h = fnv64(&f.game, sizeof f.game, h);
    if (counts[cur] < MAXF) hashes[cur][counts[cur]++] = h;
}

static void run(const char *assets, const char *scenario, double secs, int production)
{
    cur = production; counts[cur] = 0;
    memset(&f, 0, sizeof f);
    pre2_frames_run = 0;
    script_reset(scenario);
    rt_boot(assets, 1995, 6, 1);
    vga_memory_off = production;
    compositor_init();
    RtConfig cfg = { 0 };
    cfg.audio_native = production;
    rt_on_service = script_service; rt_on_display_frame = on_display; rt_on_pcm = 0;
    if (rt_init(&cfg)) exit(1);
    uint64_t end = (uint64_t)(secs * 1e9), b = 0;
    while (b < end) { b += RT_NS_PER_SEC / 60; if (rt_run_until(b) != RT_STEP_DONE) break; }
    pre2_scene_sink = 0; vga_memory_off = 0;
}

int main(int argc, char **argv)
{
    const char *assets = arg_assets(argc, argv);
    static const struct { const char *s; double secs; } cases[] = {
        { "none", 45 }, { "warp", 60 }, { "ending", 75 }, { "gameover", 75 }, { "castle", 75 }, { "attract", 110 },
    };
    int bad = 0;
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        run(assets, cases[i].s, cases[i].secs, 0);
        run(assets, cases[i].s, cases[i].secs, 1);
        unsigned k = 0;
        while (k < counts[0] && k < counts[1] && hashes[0][k] == hashes[1][k]) k++;
        int ok = counts[0] == counts[1] && k == counts[0];
        printf("%-9s %s: %u frames%s\n", cases[i].s, ok ? "equal" : "DIFFERENT", counts[0], ok ? "" : "");
        if (!ok) printf("   first different frame %u (of %u / %u)\n", k, counts[0], counts[1]);
        bad += !ok;
    }
    printf(bad ? "production_frames: production differs from the checked reference\n"
               : "production_frames: production publishes the reference-checked frames\n");
    return bad ? 1 : 0;
}
