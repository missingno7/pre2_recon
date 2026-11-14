/* Presenting more often changes nothing (docs/native.md, "Verification").
 *
 * Production configuration (video memory not modelled, native compositor). Each scenario runs
 * twice: once publishing one frame per CRT display start, once publishing four times per
 * display start and again at every 61st service point (as a high-refresh or interpolating
 * presenter would). Game memory at every step change and main-loop frame, the machine
 * counters, the PCM and the frames published at display starts must be identical: publication
 * consumes no random numbers, advances no counters and writes no game state. */
#include "script.h"
#include "c/vga.h"
#include "native/compositor.h"
#include "native/enh_render.h"

#define MAXS 20000
typedef struct { uint64_t h[MAXS]; unsigned n; uint64_t frames_hash, pcm, end, ticks; uint32_t game_frames; } Trace;
static Trace tr[2];
static Trace *cur;
static int extra;
static uint32_t last_frame, services;
static const char *last_phase;
static Pre2Frame f;

static void on_service(void)
{
    script_service();
    if (pre2_frames_run != last_frame || pre2_phase != last_phase) {
        last_frame = pre2_frames_run; last_phase = pre2_phase;
        if (cur->n < MAXS) cur->h[cur->n++] = fnv64(pre2_mem, 0xA0000u, FNV0);
    }
    if (extra && ++services % 61 == 0) compositor_publish(rt_now(), 0, &f);
}
static void on_display(uint64_t t, uint16_t start)
{
    for (int k = 0; k < (extra ? 4 : 1); k++) compositor_publish(t, start, &f);
    cur->frames_hash = fnv64(f.index, (size_t)f.w * f.h, cur->frames_hash);
    cur->frames_hash = fnv64(f.palette, sizeof f.palette, cur->frames_hash);
    if (f.game.valid) cur->frames_hash = enh_view_hash(&f.game, cur->frames_hash);
}
static void on_pcm(uint64_t t, const uint8_t *p, uint32_t n, uint32_t rate)
{
    (void)rate; cur->pcm = fnv64(&t, 8, cur->pcm); cur->pcm = fnv64(p, n, cur->pcm);
}

static void run(const char *assets, const char *scenario, double secs, int x, Trace *t)
{
    memset(t, 0, sizeof *t); t->pcm = t->frames_hash = FNV0; cur = t; extra = x;
    last_frame = services = 0; last_phase = 0; pre2_frames_run = 0;
    memset(&f, 0, sizeof f);
    script_reset(scenario);
    rt_boot(assets, 1995, 6, 1);
    vga_memory_off = 1;
    compositor_init();
    RtConfig cfg = { 0 };
    cfg.audio_native = 1;                     /* production configuration */
    rt_on_service = on_service; rt_on_display_frame = on_display; rt_on_pcm = on_pcm;
    if (rt_init(&cfg)) exit(1);
    uint64_t end = (uint64_t)(secs * 1e9), b = 0;
    while (b < end) { b += RT_NS_PER_SEC / 60; if (rt_run_until(b) != RT_STEP_DONE) break; }
    t->end = fnv64(pre2_mem, 0xA0000u, FNV0); t->ticks = rt_stats()->ticks; t->game_frames = pre2_frames_run;
    pre2_scene_sink = 0; vga_memory_off = 0;
}

int main(int argc, char **argv)
{
    const char *assets = arg_assets(argc, argv);
    static const struct { const char *s; double secs; } cases[] = {
        { "none", 45 }, { "warp", 60 }, { "gameover", 75 }, { "attract", 110 },
    };
    int bad = 0;
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        run(assets, cases[i].s, cases[i].secs, 0, &tr[0]);
        run(assets, cases[i].s, cases[i].secs, 1, &tr[1]);
        Trace *a = &tr[0], *b = &tr[1];
        int ok = a->n == b->n && !memcmp(a->h, b->h, a->n * sizeof a->h[0]) && a->end == b->end && a->pcm == b->pcm &&
                 a->ticks == b->ticks && a->game_frames == b->game_frames && a->frames_hash == b->frames_hash;
        printf("%-9s %s: %u samples, %u game frames%s\n", cases[i].s, ok ? "equal" : "DIFFERENT", a->n, a->game_frames,
               ok ? "" : (a->frames_hash != b->frames_hash ? " (published frames differ)" : " (game state differs)"));
        bad += !ok;
    }
    printf(bad ? "present_purity: presentation changed the game\n" : "present_purity: presenting more often changes nothing\n");
    return bad ? 1 : 0;
}
