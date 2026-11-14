/* Native SDL3 host: runs the translated game (c/game.c ProgramEntry) on the logical machine
 * (native/machine.c) and presents what it publishes.
 *
 *   pre2 [--assets DIR] [--smoke SECONDS] [--scenario NAME] [--turbo] [--headless] [--shots DIR]
 *        [--fast-load] [--fast-vga] [--year YYYY] [--demo-out FILE] [--demo-in FILE] [--reference]
 *        [--mode faithful|enhanced|widescreen] [--aspect 16:9|16:10|21:9|window]
 *        [--interpolate] [--smooth-camera] [--camera-tau SECONDS] [--smooth-transitions]
 *        [--hud left|centre|right] [--backdrop black|mirror|stretch] [--wide-sim WIDTH|auto]
 * Defaults are faithful, no smoothing. Smooth camera implies interpolation.
 * Presentation-only controls and transition fallbacks: docs/native-presenter.md.
 *
 * The address space starts as the program image of our own ASM build (embedded by
 * tools/hostbuild.py as pre2_image[], relocated to LOAD_SEG), never PRE2.EXE. Machine
 * configuration the original probes for: a Sound Blaster at 220h, IRQ 7 (the probes are a
 * platform service), a 386-class CPU, 640 KiB conventional memory, no EMS/XMS, the RTC date
 * from the host clock, drive C:. The BIOS ROM area F000:FFF0 reads as zero bytes (the level
 * codes hash it, so codes match those of a machine whose ROM fold is zero).
 *
 * Presentation: the native compositor (native/compositor.h) draws every frame from the game's
 * scene events and native assets; video memory is not modelled (c/vga.h vga_memory_off).
 * Audio: the native audio engine (native/audio.h) mixes from the game's audio commands.
 * --reference instead models video memory and scans the VGA model out, and runs the game's
 * own mixer through the Sound Blaster model (the parity references; diagnostics only).
 *
 * Time: the game runs in logical time (docs/native.md). In normal play the host advances the
 * logical bound with the wall clock; --turbo advances it as fast as the game runs (scripted
 * runs: the same script gives the same run). Presentation and audio output only consume what
 * the machine produced; they never drive the game.
 *
 * --smoke SECONDS: the bounded integration test (docs/smoke-test.md), in logical time:
 * scripted key presses get through the start-up screens into the first level, play a few
 * seconds, report tick and frame cadence, and stop.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../c/pre2.h"
#include "../c/vga.h"
#include "../c/game.h"
#include "../c/startup.h"
#include "../native/machine.h"
#include "sdl_host.h"
#include "../native/scanout.h"
#include "../native/compositor.h"
#include "../native/presenter.h"

static double secs(uint64_t ns) { return ns / 1e9; }

/* ---- presentation: the reference VGA scan-out at each CRT frame's display start ------------ */
static uint32_t frame_px[PRESENTER_MAX_W * PRESENTER_MAX_H];
static int frame_w = 320, frame_h = 200;
static uint64_t frames_shown;
static int reference_video;
static Pre2Frame frame;
static Presenter *presenter;
static PresenterOptions presentation;
/* Single producer (machine), single consumer (SDL). A bounded queue drops old
 * pictures when presentation falls behind. The machine never waits for vsync. */
#define FRAME_QUEUE 32
static Pre2Frame queue[FRAME_QUEUE], incoming;
static unsigned qhead, qcount;
static uint8_t keys[512];
static unsigned key_count;
static int threaded, worker_done, want_quit;
static uint64_t published_bound;
static void queue_key(uint8_t key)
{
    sdl_host_lock();
    if (key_count < sizeof keys) keys[key_count++]=key;
    sdl_host_unlock();
}
static void on_display_frame(uint64_t t, uint16_t start)
{
    if (reference_video) {
        ScanInfo si;
        scan_indices(&scan_vga,start,frame.index,&si);
        frame.t_ns=t; frame.seq++; frame.w=(uint16_t)si.w; frame.h=(uint16_t)si.h; frame.blank=(uint8_t)si.blank;
        memcpy(frame.palette,pre2_vga.dac,sizeof frame.palette); frame.pel_mask=pre2_vga.pel_mask;
        frame.screen=pre2_phase;
        if (si.blank_black) { memset(frame.palette[0],0,3); memset(frame.index,0,(size_t)si.w*si.h); frame.pel_mask=255; }
    }
    else {
        compositor_publish(t, start, &frame);
    }
    if (threaded) {
        sdl_host_lock();
        if (qcount==FRAME_QUEUE) { qhead=(qhead+1)%FRAME_QUEUE; qcount--; }
        queue[(qhead+qcount++)%FRAME_QUEUE]=frame;
        sdl_host_unlock();
    } else presenter_push(presenter,&frame);
}

static void write_bmp(const char *path, const uint32_t *px, int w, int h)
{
    FILE *f = fopen(path, "wb");
    if (!f) return;
    uint8_t hd[54] = { 'B', 'M' };
    uint32_t size = 54 + (uint32_t)(w * h * 4);
    memcpy(hd + 2, &size, 4); hd[10] = 54; hd[14] = 40;
    int32_t ww = w, hh = -h; memcpy(hd + 18, &ww, 4); memcpy(hd + 22, &hh, 4);
    hd[26] = 1; hd[28] = 32;
    fwrite(hd, 1, 54, f); fwrite(px, 4, (size_t)(w * h), f); fclose(f);
}

/* --shots DIR: the presented frame every 0.5 s of logical time as DIR/NNNN_<step>.bmp. */
static const char *shots_dir;
static const char *shown_screen;
static void shot(void)
{
    static int n;
    char path[600], step[64];
    int i = 0;
    for (const char *c = shown_screen ? shown_screen : "frame"; *c && i < 60; c++) if ((*c >= 'A' && *c <= 'Z') || (*c >= 'a' && *c <= 'z') || (*c >= '0' && *c <= '9')) step[i++] = *c;
    step[i] = 0;
    snprintf(path, sizeof path, "%s/%04d_%s.bmp", shots_dir, n++, step);
    write_bmp(path, frame_px, frame_w, frame_h);
}

/* ---- audio: PCM in logical-time order to the output stage ------------------------------------ */
static int audio_on;
static void on_pcm(uint64_t t, const uint8_t *pcm, uint32_t n, uint32_t rate)
{
    (void)t;
    if (audio_on) sdl_host_audio(pcm, n, rate);
}

/* ---- the bounded smoke test (logical time) ------------------------------------------------- */
static uint64_t smoke_ns;                 /* stop at this logical time (0: no limit) */
static uint64_t gameplay_ns, gameplay_ticks, gameplay_blocks, gameplay_pcm;
static uint32_t gameplay_frames0;
static uint32_t gameplay_mem0, gameplay_io0;
static void key_now(uint8_t code) { rt_key_at(code, rt_now()); }

/* --scenario NAME (with --smoke): 2 s into the first level, write the state that triggers a
 * high-risk transition, then follow the program through its screens with RETURN taps,
 * printing every top-level step and level change (docs/smoke-test.md):
 *   warp      CurrentLevel 3, LevelEndMode 2: the warp into bonus room 0Ah
 *   ending    EndSignal FFh: THEEND, the intro picture gate, the front end
 *   gameover  EndSignal 1: death bounce, game-over screen, the front end
 *   castle    CurrentLevel 7, LevelEndMode 1, beginner: exit sequence, CASTLE, the front end
 *   attract   no key from the front end on: the attract demo (prints the demo player's X) */
static const char *scenario;
static int scenario_poked;
static void scenario_poke(void)
{
    if (!strcmp(scenario, "warp")) { SET_DB(DS_CurrentLevel, 3); SET_DB(DS_LevelEndMode, 2); }
    else if (!strcmp(scenario, "ending")) SET_DB(DS_EndSignal, 0xFF);
    else if (!strcmp(scenario, "gameover")) SET_DB(DS_EndSignal, 1);
    else if (!strcmp(scenario, "castle")) { SET_DB(DS_CurrentLevel, 7); SET_DB(DS_LevelEndMode, 1); SET_DB(DS_byte_B198, 0); }
    printf("scenario %s: state written at %.2f s\n", scenario, secs(rt_now()));
}
static void scenario_trace(void)
{
    static const char *phase; static int level = -1;
    if (phase != pre2_phase || level != DB(DS_CurrentLevel)) {
        phase = pre2_phase; level = DB(DS_CurrentLevel);
        printf("trace %6.2f s: %-28s CurrentLevel %02X\n", secs(rt_now()), phase, level);
    }
}

static void smoke_keys(void)
{
    /* Until the first gameplay frame: RETURN tapped (0.1 s, a quick human press) every 1.2 s (start-up screens,
     * title, front end, map: each waits for a press and a release it polls). Then: hold RIGHT. */
    static uint64_t last; static int held, down;
    uint64_t now = rt_now();
    const RtStats *st = rt_stats();
    if (!gameplay_ns && pre2_frames_run > 0) {
        gameplay_ns = now; gameplay_ticks = st->ticks; gameplay_frames0 = pre2_frames_run;
        gameplay_blocks = st->audio_blocks; gameplay_pcm = st->pcm_bytes;
        gameplay_mem0 = vga_mem_accesses; gameplay_io0 = vga_io_accesses;
        if (down) { key_now(0x9C); down = 0; }
    }
    if (scenario) scenario_trace();
    if (scenario && !strcmp(scenario, "attract")) {     /* hands off from the front end on */
        static int hands_off;
        static uint16_t maxx;
        if (!strcmp(pre2_phase, "RunFrontEnd()")) hands_off = 1;
        if (DB(DS_byte_2879) == 1 && DW(DS_PlayerX) > maxx && pre2_frames_run) {
            maxx = DW(DS_PlayerX);
            if ((maxx & 0x3F) == 0 || maxx < 0x30) printf("attract demo: frame %u PlayerX %04X\n", pre2_frames_run, maxx);
        }
        if (hands_off) { if (down) { key_now(0x9C); down = 0; } return; }
    }
    if (gameplay_ns && scenario && !scenario_poked && now - gameplay_ns >= 2000000000ull) {
        if (held) { key_now(0xCD); held = 0; }
        scenario_poke(); scenario_poked = 1; last = now;
    }
    if (gameplay_ns && !scenario_poked) { if (!held) { key_now(0x4D); held = 1; } return; }
    if (down && now - last >= 100000000ull) { key_now(0x9C); down = 0; }
    /* hands off while the map scrolls in, so the whole scroll-in can be observed */
    if (!strcmp(pre2_phase, "ShowWorldMapScrollIn()")) return;
    if (!down && now - last >= 1200000000ull) { key_now(0x1C); down = 1; last = now; }
}

/* A digest of the game's memory (the program image, DATA and every segment it allocates):
 * two runs with the same script must end with the same digest. */
static uint64_t mem_digest(void)
{
    uint64_t h = 1469598103934665603ull;
    for (uint32_t i = 0; i < 0xA0000u; i++) { h ^= pre2_mem[i]; h *= 1099511628211ull; }
    return h;
}

static void smoke_report(void)
{
    const RtStats *st = rt_stats();
    double s = secs(rt_now());
    printf("smoke: ran %.2f s logical; timer interrupts %llu (%.2f Hz); display frames %llu, shown %llu\n", s,
           (unsigned long long)st->ticks, st->ticks / s, (unsigned long long)st->display_frames, (unsigned long long)frames_shown);
    if (gameplay_ns) {
        double g = secs(rt_now() - gameplay_ns);
        uint32_t f = pre2_frames_run - gameplay_frames0;
        uint64_t t = st->ticks - gameplay_ticks;
        printf("smoke: gameplay reached after %.2f s; in gameplay %.2f s: %llu ticks (%.2f Hz), %u frames "
               "(%.2f frames/s, %.2f ticks/frame); PlayerX %04X, CurrentLevel %u\n",
               secs(gameplay_ns), g, (unsigned long long)t, t / g, f, f / g,
               f ? t / (double)f : 0.0, DW(DS_PlayerX), DB(DS_CurrentLevel));
        printf("smoke: in gameplay: audio blocks %.2f/s, PCM %.0f bytes/s\n",
               (st->audio_blocks - gameplay_blocks) / g, (st->pcm_bytes - gameplay_pcm) / g);
        if (f) printf("smoke: in gameplay: VGA accesses per frame: %u memory, %u port (bus time %.1f ms of the 41.2 ms frame)\n",
                      (vga_mem_accesses - gameplay_mem0) / f, (vga_io_accesses - gameplay_io0) / f,
                      ((vga_mem_accesses - gameplay_mem0) * 0.5e-3 + (vga_io_accesses - gameplay_io0) * 1e-3) / f);
    } else printf("smoke: gameplay NOT reached\n");
    printf("smoke: audio blocks %llu (%.2f/s), PCM bytes %llu (%.0f B/s); load time %.2f s, bus time %.2f s\n",
           (unsigned long long)st->audio_blocks, st->audio_blocks / s, (unsigned long long)st->pcm_bytes, st->pcm_bytes / s,
           secs(st->load_ns), secs(st->bus_ns));
    printf("smoke: last top-level step: %s; main-loop frames %u; memory digest %016llx\n", pre2_phase, pre2_frames_run,
           (unsigned long long)mem_digest());
    write_bmp("smoke_last.bmp", frame_px, frame_w, frame_h);
    printf("smoke: last frame written to smoke_last.bmp\n");
}

/* ---- the game's own demo log (ds:003F..083E: input log, expert flag 083Dh, level 083Eh) ---
 * --demo-out FILE: at exit, save the log: after a game started from the front end it holds
 *   that game's recorded input (PollInputState records one entry per change of the six input
 *   bits, up to 7FCh bytes), in the format tests/demo.py replays on the original and the C.
 * --demo-in FILE: load a saved log after the late-year screen (which would record over it on
 *   a date from 1996 on), so the attract demo plays it.
 * --year YYYY: the RTC year the game reads (before 1996 the late-year screen, which breaks
 *   the shipped attract demo, does not appear). */
static const char *demo_in, *demo_out;
static int smoke_on;
static void demo_save(void)
{
    FILE *f = fopen(demo_out, "wb");
    if (!f) { fprintf(stderr, "demo: cannot write %s\n", demo_out); return; }
    fwrite(pre2_mem + lin(DATA_SEG, 0x3F), 1, 0x800, f);
    fclose(f);
    unsigned n = 0;
    while (n < 0x3FE && rd16(DATA_SEG, (uint16_t)(0x3F + 2 * n)) != 0x55AA) n++;
    printf("demo: saved %s (level %u, expert %u, %u entries)\n", demo_out, DB(0x083E), DB(0x083D), n);
}

static uint64_t next_shot_ns;
static void on_service(void)
{
    static int loaded;
    if (demo_in && !loaded && !strcmp(pre2_phase, "InitSoundDevices()")) {
        FILE *f = fopen(demo_in, "rb");
        if (f && fread(pre2_mem + lin(DATA_SEG, 0x3F), 1, 0x800, f) == 0x800) printf("demo: loaded %s\n", demo_in);
        else fprintf(stderr, "demo: cannot read %s (2048 bytes)\n", demo_in);
        if (f) fclose(f);
        loaded = 1;
    }
    if (smoke_on) smoke_keys();
}

typedef struct WorkerConfig { RtConfig machine; int turbo; uint64_t wall0; } WorkerConfig;
static int simulation(void *arg)
{
    WorkerConfig *cfg=arg;
    int status=0;
    RtStatus st=RT_STEP_DONE;
    uint64_t bound=0;
    if (rt_init(&cfg->machine)) { fprintf(stderr,"rt_init failed\n"); status=1; goto done; }
    for (;;) {
        if (threaded) {
            uint8_t pending[sizeof keys]; unsigned n; int quit;
            sdl_host_lock(); n=key_count; memcpy(pending,keys,n); key_count=0; quit=want_quit; sdl_host_unlock();
            for (unsigned i=0;i<n;i++) rt_key(pending[i]);
            if (quit) { rt_request_quit(); st=RT_QUIT; break; }
        }
        bound=cfg->turbo?bound+1000000000ull/60:sdl_host_ns()-cfg->wall0;
        if (smoke_ns && bound>smoke_ns) bound=smoke_ns;
        st=rt_run_until(bound);
        if (threaded) { sdl_host_lock(); published_bound=bound; sdl_host_unlock(); }
        else {
            PresenterImage im={0}; im.pixels=frame_px; im.capacity=sizeof frame_px/sizeof *frame_px;
            if (!presenter_render(presenter,bound,1.6,&im)) {
                frame_w=im.w; frame_h=im.h; shown_screen=frame.screen;
                if (shots_dir && bound>=next_shot_ns) { shot(); next_shot_ns=bound+500000000ull; }
            }
        }
        if (st!=RT_STEP_DONE) break;
        if (smoke_ns && rt_now()>=smoke_ns) { status=3; break; }
        if (!cfg->turbo) sdl_host_sleep_ns(1000000);
    }
    if (st==RT_EXITED) { fprintf(stderr,"ProgramEntry returned status %d\n",rt_exit_code()); status=rt_exit_code()?2:0; }
done:
    if (threaded) { sdl_host_lock(); worker_done=1; sdl_host_unlock(); }
    return status;
}

int main(int argc, char **argv)
{
    const char *assets = "assets";
    double smoke = 0;
    int year_override = 0, turbo = 0, headless = 0;
    const char *wide_sim = 0;       /* --wide-sim WIDTH|auto: the true-widescreen gameplay variant */
    RtConfig cfg = { 0 };
    presentation=presenter_defaults();
    cfg.audio_native = 1;                     /* native audio engine; --reference: the SB path */
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--assets") && i + 1 < argc) assets = argv[++i];
        else if (!strcmp(argv[i], "--smoke") && i + 1 < argc) smoke = atof(argv[++i]);
        else if (!strcmp(argv[i], "--shots") && i + 1 < argc) shots_dir = argv[++i];
        else if (!strcmp(argv[i], "--fast-load")) cfg.fast_load = 1;
        else if (!strcmp(argv[i], "--scenario") && i + 1 < argc) scenario = argv[++i];
        else if (!strcmp(argv[i], "--fast-vga")) cfg.fast_vga = 1;
        else if (!strcmp(argv[i], "--wide-sim") && i + 1 < argc) wide_sim = argv[++i];
        else if (!strcmp(argv[i], "--reference")) { reference_video = 1; cfg.audio_native = 0; }
        else if (!strcmp(argv[i], "--turbo")) turbo = 1;
        else if (!strcmp(argv[i], "--headless")) headless = turbo = 1;
        else if (!strcmp(argv[i], "--demo-out") && i + 1 < argc) demo_out = argv[++i];
        else if (!strcmp(argv[i], "--demo-in") && i + 1 < argc) demo_in = argv[++i];
        else if (!strcmp(argv[i], "--year") && i + 1 < argc) year_override = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--mode") && i+1<argc) {
            const char *m=argv[++i];
            if (!strcmp(m,"faithful")) presentation.mode=PRESENT_FAITHFUL;
            else if (!strcmp(m,"enhanced")) presentation.mode=PRESENT_ENHANCED;
            else if (!strcmp(m,"widescreen")) presentation.mode=PRESENT_WIDESCREEN;
            else { fprintf(stderr,"unknown presentation mode: %s\n",m); return 1; }
        }
        else if (!strcmp(argv[i],"--interpolate")) presentation.interpolate=1;
        else if (!strcmp(argv[i],"--smooth-camera")) presentation.smooth_camera=1;
        else if (!strcmp(argv[i],"--smooth-transitions")) presentation.smooth_transitions=1;
        else if (!strcmp(argv[i],"--camera-tau") && i+1<argc) {
            char *end; presentation.camera_tau=strtod(argv[++i],&end);
            if (*end || !(presentation.camera_tau>0) || presentation.camera_tau>10) { fprintf(stderr,"camera-tau must be in (0,10] seconds\n"); return 1; }
        }
        else if (!strcmp(argv[i],"--aspect") && i+1<argc) {
            const char *a=argv[++i];
            if (!strcmp(a,"16:9")) presentation.aspect=16.0/9;
            else if (!strcmp(a,"16:10")) presentation.aspect=1.6;
            else if (!strcmp(a,"21:9")) presentation.aspect=21.0/9;
            else if (!strcmp(a,"window")) presentation.aspect=0;
            else { fprintf(stderr,"unknown aspect: %s\n",a); return 1; }
        }
        else if (!strcmp(argv[i],"--hud") && i+1<argc) {
            const char *h=argv[++i];
            if (!strcmp(h,"left")) presentation.hud=ENH_HUD_LEFT;
            else if (!strcmp(h,"centre")) presentation.hud=ENH_HUD_CENTRE;
            else if (!strcmp(h,"right")) presentation.hud=ENH_HUD_RIGHT;
            else { fprintf(stderr,"unknown HUD policy: %s\n",h); return 1; }
        }
        else if (!strcmp(argv[i],"--backdrop") && i+1<argc) {
            const char *b=argv[++i];
            if (!strcmp(b,"black")) presentation.backdrop=ENH_BACK_BLACK;
            else if (!strcmp(b,"mirror")) presentation.backdrop=ENH_BACK_MIRROR;
            else if (!strcmp(b,"stretch")) presentation.backdrop=ENH_BACK_STRETCH;
            else { fprintf(stderr,"unknown backdrop policy: %s\n",b); return 1; }
        }
        else { fprintf(stderr,"unknown or incomplete option: %s\n",argv[i]); return 1; }
    }
    if (wide_sim) {
        /* True widescreen simulates a wider view (docs/native-widescreen.md); only the
         * widescreen presentation can show it. `auto` takes the width from the aspect
         * (200 rows), rounded to 16 pixels, within 320..640. */
        unsigned w = (unsigned)atoi(wide_sim);
        if (!strcmp(wide_sim, "auto")) {
            double a = presentation.aspect > 0 ? presentation.aspect : 16.0 / 9.0;
            w = (unsigned)(200.0 * a / 16.0 + 0.5) * 16u;
            if (w < 320) w = 320;
            if (w > 640) w = 640;
        }
        if (rt_set_wide_sim_width(w)) { fprintf(stderr, "--wide-sim requires auto or a multiple of 16 from 320 to 640\n"); return 1; }
        if (presentation.mode != PRESENT_WIDESCREEN) { presentation.mode = PRESENT_WIDESCREEN; fprintf(stderr, "--wide-sim: widescreen presentation\n"); }
        printf("true widescreen: simulated view %u px wide\n", w);
    }
    time_t t = time(NULL); struct tm *lt = localtime(&t);
    rt_boot(assets, year_override ? year_override : lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday);
    if (!reference_video) { vga_memory_off = 1; compositor_init(); }
    if (sdl_host_init(!headless, !turbo)) return 1;
    if (reference_video) presentation=presenter_defaults();
    presenter=presenter_create(&presentation);
    if (!presenter) return 1;
    audio_on = !turbo;
    if (smoke > 0) { smoke_ns = (uint64_t)(smoke * 1e9); smoke_on = 1; }
    rt_on_display_frame = on_display_frame;
    rt_on_pcm = on_pcm;
    rt_on_service = on_service;
    WorkerConfig worker_cfg={cfg,turbo,sdl_host_ns()};
    int status;
    threaded=!headless;
    if (!threaded) status=simulation(&worker_cfg);
    else {
        if (sdl_host_worker_start(simulation,&worker_cfg)) return 1;
        for (;;) {
            if (sdl_host_pump(queue_key)) { sdl_host_lock(); want_quit=1; sdl_host_unlock(); }
            /* Short copies only under the lock. Every raster and SDL call is
             * outside it; slow presentation can only overflow the frame queue. */
            unsigned available;
            sdl_host_lock(); available=qcount; sdl_host_unlock();
            while (available--) {
                sdl_host_lock();
                incoming=queue[qhead]; qhead=(qhead+1)%FRAME_QUEUE; qcount--;
                sdl_host_unlock();
                presenter_push(presenter,&incoming); shown_screen=incoming.screen;
            }
            uint64_t bound; int done; unsigned pending;
            sdl_host_lock(); bound=published_bound; done=worker_done; pending=qcount; sdl_host_unlock();
            PresenterImage im={0}; im.pixels=frame_px; im.capacity=sizeof frame_px/sizeof *frame_px;
            if (!presenter_render(presenter,bound,sdl_host_window_aspect(),&im)) {
                frame_w=im.w; frame_h=im.h;
                sdl_host_show(frame_px,frame_w,frame_h,presentation.mode==PRESENT_WIDESCREEN?frame_w:320,
                              presentation.mode==PRESENT_WIDESCREEN?frame_h:200); frames_shown++;
                if (shots_dir && bound>=next_shot_ns) { shot(); next_shot_ns=bound+500000000ull; }
            } else sdl_host_sleep_ns(1000000);
            if (done && !pending) break;
        }
        status=sdl_host_worker_join();
    }
    if (smoke_on) smoke_report();
    if (demo_out) demo_save();
    presenter_destroy(presenter);
    sdl_host_shutdown();
    return status;
}
