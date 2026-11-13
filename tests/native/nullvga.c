/* Video memory is presentation only: the game run with the VGA memory model off (production:
 * accesses counted for bus time, writes dropped, reads 0) must make exactly the same decisions
 * as with it on. For each scenario the whole program memory below A0000h is hashed at every
 * main-loop frame and at every top-level step change, and the sequences must be equal, as must
 * the end state, the machine counters and the PCM the game's mixer produced. */
#include "script.h"
#include "c/vga.h"

#define MAXS 20000
typedef struct { uint64_t h[MAXS]; const char *ph[MAXS]; unsigned n; uint64_t pcm, end, ticks, blocks; uint32_t frames; } Trace;
static Trace traces[2];
static Trace *cur;
static uint32_t last_frame; static const char *last_phase;

static int dump_at = -1, dump_mode;
static void sample(void)
{
    if ((int)cur->n == dump_at) {
        char name[64]; snprintf(name, sizeof name, "build/native/nullvga_%d.bin", dump_mode);
        FILE *f = fopen(name, "wb"); if (f) { fwrite(pre2_mem, 1, 0xA0000u, f); fclose(f); }
    }
    if (getenv("NULLVGA_TRACE") && cur->n < 12) printf("  mode %d sample %u %-28s t %.4f mem %u io %u ticks %u\n", dump_mode, cur->n, pre2_phase,
        rt_now() / 1e9, vga_mem_accesses, vga_io_accesses, (unsigned)rt_stats()->ticks);
    if (cur->n < MAXS) cur->ph[cur->n] = pre2_phase;
    if (cur->n < MAXS) cur->h[cur->n++] = fnv64(pre2_mem, 0xA0000u, fnv64(&pre2_frames_run, 4, FNV0));
}
static void on_service(void)
{
    script_service();
    if (pre2_frames_run != last_frame || pre2_phase != last_phase) { last_frame = pre2_frames_run; last_phase = pre2_phase; sample(); }
}
static void on_pcm(uint64_t t, const uint8_t *p, uint32_t n, uint32_t rate)
{
    (void)rate; cur->pcm = fnv64(&t, 8, cur->pcm); cur->pcm = fnv64(p, n, cur->pcm);
}

static void run(const char *assets, const char *scenario, int memory_off, double seconds, Trace *t)
{
    memset(t, 0, sizeof *t); t->pcm = FNV0; cur = t; dump_mode = memory_off;
    last_frame = 0; last_phase = 0;
    pre2_frames_run = 0;
    script_reset(scenario);
    rt_boot(assets, 1995, 6, 1);
    vga_memory_off = memory_off;
    RtConfig cfg = { 0 };
    rt_on_service = on_service; rt_on_pcm = on_pcm;
    if (rt_init(&cfg)) exit(1);
    uint64_t end = (uint64_t)(seconds * 1e9), b = 0;
    while (b < end) { b += RT_NS_PER_SEC / 60; if (rt_run_until(b) != RT_STEP_DONE) break; }
    t->end = fnv64(pre2_mem, 0xA0000u, FNV0);
    t->ticks = rt_stats()->ticks; t->blocks = rt_stats()->audio_blocks; t->frames = pre2_frames_run;
    vga_memory_off = 0;
}

int main(int argc, char **argv)
{
    const char *assets = arg_assets(argc, argv);
    if (getenv("NULLVGA_DUMP")) dump_at = atoi(getenv("NULLVGA_DUMP"));   /* diagnostics: memory at sample K */
    static const struct { const char *s; double secs; } cases[] = {
        { "none", 45 }, { "warp", 60 }, { "ending", 75 }, { "gameover", 75 }, { "castle", 75 }, { "attract", 110 },
    };
    int bad = 0;
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        run(assets, cases[i].s, 0, cases[i].secs, &traces[0]);
        run(assets, cases[i].s, 1, cases[i].secs, &traces[1]);
        Trace *a = &traces[0], *b = &traces[1];
        unsigned first = 0;
        while (first < a->n && first < b->n && a->h[first] == b->h[first]) first++;
        int ok = a->n == b->n && first == a->n && a->end == b->end && a->pcm == b->pcm && a->ticks == b->ticks &&
                 a->blocks == b->blocks && a->frames == b->frames;
        printf("%-9s %s: %u samples, %u game frames, ticks %llu, audio blocks %llu%s\n", cases[i].s, ok ? "equal" : "DIFFERENT",
               a->n, a->frames, (unsigned long long)a->ticks, (unsigned long long)a->blocks, ok ? "" : "");
        if (!ok && first < a->n) printf("   at step %s (sample %u); previous step %s\n", a->ph[first], first, first ? a->ph[first - 1] : "-");
        if (!ok) { printf("   first different sample %u of %u/%u; end %s; pcm %s\n", first, a->n, b->n,
                          a->end == b->end ? "equal" : "different", a->pcm == b->pcm ? "equal" : "different"); bad++; }
    }
    printf(bad ? "nullvga: game state depends on video memory\n" : "nullvga: video memory is presentation only in every scenario\n");
    return bad ? 1 : 0;
}
