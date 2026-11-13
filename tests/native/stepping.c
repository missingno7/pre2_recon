/* The timing boundary: a run is a function of its input schedule, not of how the host steps
 * it. The same scripted 40 s run (start-up screens, front end, map, level 1 with RIGHT held)
 * is stepped three ways: 1/60 s slices (a 60 Hz presenter), 1/240 s slices (a 240 Hz
 * presenter) and irregular slices from 0.1 ms to 80 ms (a stalling host). The game's memory,
 * the VGA state, the machine counters and the scripted observations must be identical. */
#include "c/pre2.h"
#include "c/vga.h"
#include "c/game.h"
#include "native/machine.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static uint64_t last; static int held, down;
static void script(void)
{
    uint64_t now = rt_now();
    if (pre2_frames_run > 0) {
        if (down) { rt_key_at(0x9C, now); down = 0; }
        if (!held) { rt_key_at(0x4D, now); held = 1; }
        return;
    }
    if (down && now - last >= 100000000ull) { rt_key_at(0x9C, now); down = 0; }
    if (!down && now - last >= 1200000000ull) { rt_key_at(0x1C, now); down = 1; last = now; }
}

static uint64_t fnv(const uint8_t *p, size_t n, uint64_t h)
{
    for (size_t i = 0; i < n; i++) { h ^= p[i]; h *= 1099511628211ull; }
    return h;
}

typedef struct { uint64_t mem, vga, ticks, blocks, frames, pcm; uint32_t game_frames; } Result;
static uint64_t pcm_hash;
static void on_pcm(uint64_t t, const uint8_t *pcm, uint32_t n, uint32_t rate)
{
    pcm_hash = fnv((const uint8_t *)&t, sizeof t, pcm_hash);
    pcm_hash = fnv(pcm, n, pcm_hash); (void)rate;
}

static Result run(const char *assets, int mode)
{
    last = 0; held = down = 0; pcm_hash = 1469598103934665603ull;
    pre2_frames_run = 0;
    rt_boot(assets, 1995, 6, 1);
    RtConfig cfg = { 0 };
    rt_on_service = script;
    rt_on_pcm = on_pcm;
    if (rt_init(&cfg)) { printf("rt_init failed\n"); exit(1); }
    const uint64_t end = 40ull * RT_NS_PER_SEC;
    uint64_t bound = 0, seed = 12345;
    while (bound < end) {
        if (mode == 0) bound += RT_NS_PER_SEC / 60;
        else if (mode == 1) bound += RT_NS_PER_SEC / 240;
        else { seed = seed * 6364136223846793005ull + 1442695040888963407ull; bound += 100000ull + (seed >> 33) % 80000000ull; }
        if (bound > end) bound = end;
        if (rt_run_until(bound) != RT_STEP_DONE) { printf("game stopped\n"); exit(1); }
    }
    Result r; memset(&r, 0, sizeof r);
    r.mem = fnv(pre2_mem, 0xA0000u, 1469598103934665603ull);
    r.vga = fnv((const uint8_t *)&pre2_vga, sizeof pre2_vga, 1469598103934665603ull);
    const RtStats *s = rt_stats();
    r.ticks = s->ticks; r.blocks = s->audio_blocks; r.frames = s->display_frames; r.pcm = pcm_hash;
    r.game_frames = pre2_frames_run;
    printf("mode %d: now %.3f s, game frames %u, ticks %llu, audio blocks %llu, display frames %llu, mem %016llx vga %016llx pcm %016llx\n",
           mode, rt_now() / 1e9, r.game_frames, (unsigned long long)r.ticks, (unsigned long long)r.blocks,
           (unsigned long long)r.frames, (unsigned long long)r.mem, (unsigned long long)r.vga, (unsigned long long)r.pcm);
    return r;
}

int main(int argc, char **argv)
{
    const char *assets = argc > 2 ? argv[2] : "assets";
    Result a = run(assets, 0), b = run(assets, 1), c = run(assets, 2);
    int ok = a.game_frames > 100 && !memcmp(&a, &b, sizeof a) && !memcmp(&a, &c, sizeof a);
    printf(ok ? "stepping: identical under 60 Hz, 240 Hz and irregular stepping\n" : "stepping: MISMATCH\n");
    return ok ? 0 : 1;
}
