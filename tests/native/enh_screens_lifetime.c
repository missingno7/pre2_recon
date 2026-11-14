/* Production-hook lifetime and steady-state capture cost. Retained screen data
 * must survive both a 32-publication delay and replacement of its whole session. */
#include "script.h"
#include "c/scene_screens.h"
#include "c/scene_game.h"
#include "c/vga.h"
#include "native/compositor.h"
#include "native/enh_screens.h"
#include <assert.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif
#undef assert
#define assert(test) do { if (!(test)) { fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#test); exit(1); } } while (0)
#define HISTORY 32
#define BYTE_LIMIT (32u * 1024u * 1024u)
#define BLOCK_LIMIT 50000u
static Pre2Frame frame, history[HISTORY], pinned[3];
static unsigned cursor, occupied, checked, publications, peak_window, wide_checks;
static uint64_t wide_expected[HISTORY];
static uint8_t output[FRAME_MAX_W * FRAME_MAX_H];
static void (*sink)(const SceneEvent *);
static uint64_t gameover_start;
static const char *scenario;
static int benchmark, in_world;
static SceneEvent sample[4096];
static unsigned sample_count;
static uint64_t game_events;
static double game_start, game_end;
static double seconds(void)
{
#ifdef _WIN32
    LARGE_INTEGER count, frequency; QueryPerformanceCounter(&count); QueryPerformanceFrequency(&frequency);
    return (double)count.QuadPart / (double)frequency.QuadPart;
#else
    struct timespec t; timespec_get(&t,TIME_UTC); return t.tv_sec + t.tv_nsec * 1e-9;
#endif
}
static void equal(const Pre2Frame *f)
{
    if (!f->screen_view.valid) return;
    EnhImage im = {0}; im.index = output; im.capacity = sizeof output; im.stride = f->w;
    assert(!enh_screens_render(&f->screen_view,f->palette,f->pel_mask,
        f->screen_view.pan_x,f->screen_view.pan_y,f->w,f->h,&im));
    assert(!memcmp(output,f->index,(size_t)f->w*f->h)); checked++;
}
static uint64_t wide_hash(const Pre2Frame *f)
{
    if (!f->screen_view.valid || (f->screen_view.kind != FRAME_SCREEN_MENU &&
        f->screen_view.kind != FRAME_SCREEN_WORLD_MAP)) return 0;
    EnhImage im = {0}; im.index=output; im.capacity=sizeof output; im.stride=640;
    assert(!enh_screens_render(&f->screen_view,f->palette,f->pel_mask,
        f->screen_view.pan_x,f->screen_view.pan_y,640,200,&im));
    return fnv64(output,640*200,FNV0);
}
static void display(uint64_t t,uint16_t start)
{
    compositor_publish(t,start,&frame); publications++;
    int slot = frame.w == 640 ? 2 : frame.screen_view.kind == FRAME_SCREEN_MENU ? 1 :
        frame.screen_view.kind == FRAME_SCREEN_WORLD_MAP && frame.screen_view.pan_x > 300 ? 0 : -1;
    if (slot >= 0 && frame.screen_view.valid && !pinned[slot].screen_view.valid) {
        pinned[slot] = frame; enh_screen_view_retain(&pinned[slot].screen_view);
    }
    in_world = frame.game.valid && frame.game.world_valid && !frame.transition.kind;
    if (occupied == HISTORY) {
        equal(&history[cursor]);
        if (wide_expected[cursor]) { assert(wide_hash(&history[cursor])==wide_expected[cursor]); wide_checks++; }
        enh_screen_view_release(&history[cursor].screen_view);
    } else occupied++;
    history[cursor] = frame; enh_screen_view_retain(&history[cursor].screen_view);
    wide_expected[cursor] = wide_hash(&frame);
    cursor = (cursor + 1) % HISTORY;
    if (occupied > peak_window) peak_window = occupied;
    EnhScreenMemoryStats stats = enh_screen_memory_stats();
    if (publications > 64) { assert(stats.live_bytes < BYTE_LIMIT); assert(stats.live_blocks < BLOCK_LIMIT); }
}
static void event(const SceneEvent *e)
{
    sink(e);
    if (e->kind == SCENE_SCREEN_PICTURE && e->a[2] == 0x5e80) gameover_start = rt_now();
    if (benchmark && in_world && pre2_frames_run >= 10 && e->kind >= SCENE_GAME_ASSETS && e->kind <= SCENE_GAME_HIDE) {
        if (!game_events++) game_start = seconds();
        game_end = seconds();
        /* Skip resource/transition boundaries in the replay microbenchmark.
         * Their real once-per-handoff cost remains in the whole run. */
        if (sample_count < sizeof sample / sizeof *sample &&
            e->kind != SCENE_GAME_ASSETS && e->kind != SCENE_GAME_LEVEL &&
            e->kind != SCENE_GAME_TRANSITION && e->kind != SCENE_GAME_HIDE &&
            !(e->kind == SCENE_GAME_COPY && e->a[2] == 8000)) sample[sample_count++] = *e;
    }
}
static void service(void)
{
    if (!strcmp(scenario,"gameover") && gameover_start && rt_now()-gameover_start<12000000000ull) {
        if (script.down) { rt_key_at(0x9c,rt_now()); script.down=0; }
        return;
    }
    script_service();
}
static void flush(void)
{
    for (unsigned i=0;i<occupied;i++) {
        equal(&history[i]);
        if (wide_expected[i]) { assert(wide_hash(&history[i])==wide_expected[i]); wide_checks++; }
        enh_screen_view_release(&history[i].screen_view);
    }
    memset(history,0,sizeof history); occupied=cursor=0;
}
static void run(const char *assets,const char *name,unsigned duration,int perf)
{
    scenario=name; benchmark=perf; publications=0; gameover_start=0; in_world=0;
    game_events=0; sample_count=0; game_start=game_end=0;
    script_reset(name); pre2_frames_run=0; rt_boot(assets,1995,6,1);
    /* Keep the preceding run's 32 publications through this session reset.
     * Every old view will be rendered before eviction by this run's first 32. */
    compositor_init();
    for (unsigned i=0;i<3;i++) equal(&pinned[i]);
    enh_screen_memory_reset_peaks(); vga_memory_off=1;
    sink=pre2_scene_sink; pre2_scene_sink=event;
    rt_on_service=service; rt_on_display_frame=display; rt_on_pcm=0;
    RtConfig cfg={0}; cfg.audio_native=1; assert(!rt_init(&cfg));
    for (uint64_t b=0;b<(uint64_t)duration*RT_NS_PER_SEC;b+=RT_NS_PER_SEC/60) {
        if (rt_run_until(b)!=RT_STEP_DONE || (perf && pre2_frames_run>=75)) break;
    }
    EnhScreenMemoryStats stats=enh_screen_memory_stats();
    printf("screen retention %s %us: %u publications, peak %zu bytes / %zu blocks; live %zu bytes / %zu blocks, checked %u\n",
        name,duration,publications,stats.peak_bytes,stats.peak_blocks,stats.live_bytes,stats.live_blocks,checked);
    assert(stats.peak_bytes<BYTE_LIMIT && stats.peak_blocks<BLOCK_LIMIT);
    if (perf) {
        assert(sample_count>100 && game_end>game_start);
        uint64_t before=fnv64(pre2_mem,0xa0000,FNV0);
        double begin=seconds();
        const unsigned repetitions=256;
        for (unsigned r=0;r<repetitions;r++) for (unsigned i=0;i<sample_count;i++) draw_screens_observe(&sample[i]);
        double elapsed=seconds()-begin;
        assert(before==fnv64(pre2_mem,0xa0000,FNV0));
        double ns=elapsed*1e9/(sample_count*repetitions), rate=game_events/(game_end-game_start);
        EnhScreenObserveStats observe = enh_screen_observe_stats();
        printf("screen observe counters: %llu events / %llu skipped / %llu handoffs\n",
            (unsigned long long)observe.events,(unsigned long long)observe.skipped,(unsigned long long)observe.handoffs);
        printf("screen observe: %llu gameplay events, %.0f events/s; replay %u x %u events, %.1f ns/event, estimated %.3f%% wall share\n",
            (unsigned long long)game_events,rate,repetitions,sample_count,ns,rate*ns/1e7);
    }
    pre2_scene_sink=0; vga_memory_off=0;
}
int main(int argc,char **argv)
{
    setvbuf(stdout,0,_IONBF,0);
    const char *assets=arg_assets(argc,argv);
    /* --bench-only is used for the before/after overhead measurement. */
    if (argc>1 && !strcmp(argv[1],"--bench-only")) run(assets,"none",90,1);
    else {
        run(assets,"attract",3600,0); run(assets,"ending",240,0);
        run(assets,"gameover",240,0); run(assets,"attract",600,0);
        run(assets,"none",90,1);
    }
    flush(); compositor_init();
    for (unsigned i=0;i<3;i++) { equal(&pinned[i]); enh_screen_view_release(&pinned[i].screen_view); }
    EnhScreenMemoryStats stats=enh_screen_memory_stats();
    assert(!stats.live_bytes && !stats.live_blocks && !stats.live_images && !stats.live_layers);
    printf("screen retention: %u retained frames and %u wide images exact, window %u, reset/release returns all heap counts to zero\n",checked,wide_checks,peak_window);
    return 0;
}
