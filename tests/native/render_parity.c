/* Pixel parity of the native compositor with the reference (docs/native.md, "Verification").
 *
 * Runs the scripted scenarios (tests/native/script.h) with video memory modelled, so the
 * game's own raster code draws into the VGA model while the compositor draws the same scene
 * from the events. At every CRT display start both are scanned out with the same latched start
 * address and register state (native/scanout.h) and the DAC-index images must be equal.
 * Per top-level step the number of frames and of differing frames is reported.
 *
 *   render_parity [--assets DIR] [--scenario NAME] [--seconds S] [--store] [--dump DIR]
 *     --store   also compare the whole video store with the VGA planes at every display frame
 *               and report the first differing address (localises a missing or wrong event)
 *     --dump    write the first differing frame of each step as DIR/<step>_<n>_{ref,nat}.bmp
 * Exit status 0 only when every frame of every scenario is equal.
 */
#include "script.h"
#include "c/vga.h"
#include "native/compositor.h"
#include "native/scanout.h"

#define MAX_STEPS 64
typedef struct { const char *name; uint32_t frames, bad, store_bad; uint32_t first_bad_frame, first_bad_px; uint32_t first_store_addr; } StepStat;
static StepStat steps[MAX_STEPS];
static int nsteps;
static int opt_store;
static const char *dump_dir;
static Pre2Frame nat;
static uint8_t ref_idx[SCAN_MAX_W * SCAN_MAX_H];
static uint32_t frame_no;

static StepStat *step_for(const char *name)
{
    for (int i = 0; i < nsteps; i++) if (steps[i].name == name || !strcmp(steps[i].name, name)) return &steps[i];
    if (nsteps == MAX_STEPS) return &steps[MAX_STEPS - 1];
    steps[nsteps].name = name;
    return &steps[nsteps++];
}

static void bmp(const char *path, const uint8_t *idx, int w, int h)
{
    FILE *f = fopen(path, "wb");
    if (!f) return;
    uint8_t hd[54] = { 'B', 'M' };
    uint32_t size = 54 + (uint32_t)(w * h * 4);
    memcpy(hd + 2, &size, 4); hd[10] = 54; hd[14] = 40;
    int32_t ww = w, hh = -h; memcpy(hd + 18, &ww, 4); memcpy(hd + 22, &hh, 4);
    hd[26] = 1; hd[28] = 32;
    fwrite(hd, 1, 54, f);
    for (int i = 0; i < w * h; i++) {
        const uint8_t *c = pre2_vga.dac[idx[i] & pre2_vga.pel_mask];
        uint8_t px[4] = { (uint8_t)(c[2] << 2), (uint8_t)(c[1] << 2), (uint8_t)(c[0] << 2), 255 };
        fwrite(px, 1, 4, f);
    }
    fclose(f);
}

static void on_display(uint64_t t, uint16_t start)
{
    frame_no++;
    ScanInfo si;
    scan_indices(&scan_vga, start, ref_idx, &si);
    compositor_publish(t, start, &nat);
    StepStat *s = step_for(pre2_phase);
    s->frames++;
    uint32_t diff = 0;
    if (nat.w != si.w || nat.h != si.h) diff = 1u << 30;
    else for (int i = 0; i < si.w * si.h; i++) diff += nat.index[i] != ref_idx[i];
    if (diff) {
        if (!s->bad++) {
            s->first_bad_frame = frame_no; s->first_bad_px = diff;
            if (dump_dir) {
                char p[600], clean[64]; int k = 0;
                for (const char *c = s->name; *c && k < 60; c++) if ((*c >= 'A' && *c <= 'Z') || (*c >= 'a' && *c <= 'z') || (*c >= '0' && *c <= '9')) clean[k++] = *c;
                clean[k] = 0;
                snprintf(p, sizeof p, "%s/%s_%u_ref.bmp", dump_dir, clean, frame_no); bmp(p, ref_idx, si.w, si.h);
                snprintf(p, sizeof p, "%s/%s_%u_nat.bmp", dump_dir, clean, frame_no); bmp(p, nat.index, nat.w, nat.h);
            }
        }
    }
    if (opt_store) {
        uint32_t a = 0, n = compositor_diff_vga(&a);
        if (n && !s->store_bad++) s->first_store_addr = a;
    }
}

static int run(const char *assets, const char *scenario, double seconds)
{
    nsteps = 0; frame_no = 0; memset(steps, 0, sizeof steps);
    pre2_frames_run = 0;
    script_reset(scenario);
    rt_boot(assets, 1995, 6, 1);
    compositor_init();
    RtConfig cfg = { 0 };
    rt_on_service = script_service; rt_on_display_frame = on_display; rt_on_pcm = 0;
    if (rt_init(&cfg)) exit(1);
    uint64_t end = (uint64_t)(seconds * 1e9), b = 0;
    while (b < end) { b += RT_NS_PER_SEC / 60; if (rt_run_until(b) != RT_STEP_DONE) break; }
    pre2_scene_sink = 0;
    uint32_t total = 0, bad = 0;
    printf("%s (%.0f s, %u display frames):\n", scenario, seconds, frame_no);
    for (int i = 0; i < nsteps; i++) {
        StepStat *s = &steps[i];
        total += s->frames; bad += s->bad;
        printf("  %-30s %6u frames %6u differ", s->name, s->frames, s->bad);
        if (s->bad) printf("  (first: frame %u, %u px)", s->first_bad_frame, s->first_bad_px);
        if (opt_store && s->store_bad) printf("  store differs at %u frames (first address %04Xh)", s->store_bad, s->first_store_addr);
        printf("\n");
    }
    uint16_t k; uint32_t un = compositor_unhandled(&k);
    if (un) printf("  unhandled events: %u (first kind %u)\n", un, k);
    return bad == 0;
}

int main(int argc, char **argv)
{
    const char *assets = arg_assets(argc, argv), *only = 0;
    double secs = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--store")) opt_store = 1;
        else if (!strcmp(argv[i], "--dump") && i + 1 < argc) dump_dir = argv[++i];
        else if (!strcmp(argv[i], "--scenario") && i + 1 < argc) only = argv[++i];
        else if (!strcmp(argv[i], "--seconds") && i + 1 < argc) secs = atof(argv[++i]);
    }
    static const struct { const char *s; double secs; } cases[] = {
        { "none", 45 }, { "warp", 60 }, { "ending", 75 }, { "gameover", 75 }, { "castle", 75 }, { "attract", 110 },
    };
    int ok = 1;
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (only && strcmp(only, cases[i].s)) continue;
        ok &= run(assets, cases[i].s, secs > 0 ? secs : cases[i].secs);
    }
    printf(ok ? "render_parity: every frame equal\n" : "render_parity: frames differ\n");
    return ok ? 0 : 1;
}
