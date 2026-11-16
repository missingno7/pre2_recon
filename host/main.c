/* Native SDL3 host: runs the translated game (c/game.c ProgramEntry) on the logical machine
 * (native/machine.c) and presents what it publishes.
 *
 *   pre2 [--assets DIR] [--smoke SECONDS] [--scenario NAME] [--turbo] [--headless] [--shots DIR]
 *        [--shot-interval SECONDS]
 *        [--fast-load] [--fast-vga] [--year YYYY] [--demo-out FILE] [--demo-in FILE] [--reference]
 *        [--mode faithful|enhanced|widescreen] [--aspect 16:9|16:10|21:9|window]
 *        [--interpolate] [--smooth-camera] [--camera-tau SECONDS] [--smooth-transitions]
 *        [--hud anchored|left|centre|right] [--backdrop black|mirror|stretch] [--wide-sim off|WIDTH|auto]
 *        [--settings PATH] [--fullscreen on|off] [--window-scale 1..6] [--vsync on|off]
 *        [--frame-cap 0..1000] [--integer-scaling on|off] [--pixel-aspect square|crt]
 *        [--volume 0..100] [--mute on|off] [--performance on|off]
 * F10: modal settings. Product options also accept --KEY VALUE with hyphens.
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
#include "../c/view.h"
#include "../native/machine.h"
#include "sdl_host.h"
#include "../native/scanout.h"
#include "../native/compositor.h"
#include "../native/presenter.h"
#include "../native/enh_screens.h"
#include "../native/settings.h"
#include "../native/overlay.h"
#include "../native/host_clock.h"

static double secs(uint64_t ns) { return ns / 1e9; }

/* The product is a window-subsystem program (no console of its own). Started from a
 * console without redirection, reports still go to that console. */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static void console_attach(void)
{
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if ((out && out != INVALID_HANDLE_VALUE) || !AttachConsole(ATTACH_PARENT_PROCESS)) return;
    freopen("CONOUT$", "w", stdout); freopen("CONOUT$", "w", stderr);
}
#else
static void console_attach(void) {}
#endif

/* ---- presentation: the reference VGA scan-out at each CRT frame's display start ------------ */
static uint32_t frame_px[PRESENTER_MAX_W * PRESENTER_MAX_H];
static int frame_w = 320, frame_h = 200;
static uint64_t frames_shown, draw_ns, draws;
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
static uint64_t published_now,published_ticks;
static int worker_paused;
static HostClock host_clock;
static Pre2Settings settings,saved_settings;
static char settings_path[4096];
static Pre2Overlay overlay;
static Pre2Frame history[FRAME_QUEUE];
static unsigned history_head,history_count;
static uint32_t frozen_px[PRESENTER_MAX_W*PRESENTER_MAX_H];
static int frozen_w,frozen_h;
static double frozen_aspect;
static uint64_t frozen_bound,last_present_bound;
static uint32_t *output_px;
static size_t output_capacity;

static void remember(const Pre2Frame *f)
{
    /* The rebuild history owns references to its frames' native screen assets. */
    if(history_count==FRAME_QUEUE) {
        enh_screen_view_release(&history[history_head].screen_view);
        history_head=(history_head+1)%FRAME_QUEUE; history_count--;
    }
    history[(history_head+history_count)%FRAME_QUEUE]=*f;
    enh_screen_view_retain(&history[(history_head+history_count++)%FRAME_QUEUE].screen_view);
}
static int rebuild_presenter(void)
{
    PresenterOptions opt=reference_video?presenter_defaults():settings_presenter(&settings);
    Presenter *next=presenter_create(&opt);
    if(!next) {fprintf(stderr,"cannot rebuild presenter\n"); return -1;}
    for(unsigned i=0;i<history_count;i++) presenter_push(next,&history[(history_head+i)%FRAME_QUEUE]);
    presenter_destroy(presenter); presenter=next; presentation=opt; return 0;
}
/* The output canvas: widescreen's shape is that of its 200-row images. A picture of
 * another size (the 640x480 ending) is fitted to the same canvas instead of moving the
 * letterbox for its own rounding of the width. */
static int canvas_w=320;
static double image_aspect(int w,int h)
{
    if(presentation.mode!=PRESENT_WIDESCREEN) canvas_w=320;
    else if(h==200) canvas_w=w;
    return settings.pixel_aspect?canvas_w/240.0:canvas_w/200.0;
}
static double display_aspect(void) { return image_aspect(frame_w,frame_h); }
static int ui_key(int code,int down,int repeat,int shift)
{
    int was_open=overlay.open; OverlayKey key=OVERLAY_ENTER; int recognized=1;
    if(code==0x44) {if(!down || repeat) return 1; key=OVERLAY_TOGGLE;}
    else {
        if(!was_open || !down) return was_open;
        switch(code) {
        case 1:key=OVERLAY_ESCAPE;break; case 0x48:key=OVERLAY_UP;break;
        case 0x50:key=OVERLAY_DOWN;break; case 0x4b:key=OVERLAY_LEFT;break;
        case 0x4d:key=OVERLAY_RIGHT;break; case 0x1c:key=OVERLAY_ENTER;break;
        case 0x0f:key=shift?OVERLAY_BACKTAB:OVERLAY_TAB;break; default:recognized=0;break;
        }
    }
    Pre2Settings before=settings;
    Pre2Setting changed=recognized?overlay_key(&overlay,&settings,key):SET_COUNT;
    if(changed!=SET_COUNT) {
        if(changed<=SET_BACKDROP && rebuild_presenter()) {settings=before; return 1;}
        settings_copy(&saved_settings,&settings,changed);
        if(*settings_path) settings_save(&saved_settings,settings_path,stderr);
        else fprintf(stderr,"settings: preferences unavailable; change applies only to this run\n");
        sdl_host_apply(&settings);
        if(changed<=SET_BACKDROP) {
            PresenterImage im={0}; im.pixels=frozen_px; im.capacity=sizeof frozen_px/sizeof *frozen_px;
            if(!presenter_render(presenter,frozen_bound,sdl_host_window_aspect(),&im)) {
                frozen_w=im.w; frozen_h=im.h;
            }
        }
        frozen_aspect=image_aspect(frozen_w,frozen_h);
    }
    if(was_open!=overlay.open) {
        if(overlay.open) {
            sdl_host_lock(); host_clock_pause(&host_clock,1,sdl_host_ns()); sdl_host_unlock();
            /* Acknowledge at the next worker boundary, after PCM submission.
             * No rt_run_until call can start until the modal UI closes. */
            for(;;) {
                int stopped; sdl_host_lock(); stopped=worker_paused || worker_done; sdl_host_unlock();
                if(stopped) break;
                sdl_host_sleep_ns(1000000);
            }
            sdl_host_audio_pause(1);
            frozen_w=frame_w; frozen_h=frame_h; frozen_aspect=display_aspect();
            frozen_bound=last_present_bound;
            memcpy(frozen_px,frame_px,(size_t)frame_w*frame_h*4);
        } else {
            sdl_host_audio_pause(0);
            sdl_host_lock(); host_clock_pause(&host_clock,0,sdl_host_ns()); sdl_host_unlock();
        }
    }
    return was_open || overlay.open;
}
/* The game image is always scaled by the GPU. The menu and the performance line are drawn
 * at the drawable's own resolution (integral authored font) into an overlay the renderer
 * blends on top: nothing of the window's size is composed on the CPU per frame except the
 * overlay that is actually visible. */
static void show_product(const OverlayPerf *perf)
{
    const uint32_t *src=overlay.open?frozen_px:frame_px;
    int sw=overlay.open?frozen_w:frame_w,sh=overlay.open?frozen_h:frame_h;
    (void)image_aspect(sw,sh);
    int aw=canvas_w,ah=settings.pixel_aspect?240:200;
    if(!overlay.open && !settings.performance) { sdl_host_show(src,sw,sh,aw,ah,NULL,0,0); return; }
    int w,h; sdl_host_window_size(&w,&h);
    if(w<=0 || h<=0 || w>8192 || h>8192) return;
    int scale=overlay_scale(w,h),rows=overlay.open?h:overlay_performance_rows(scale);
    if(rows>h) rows=h;
    size_t n=(size_t)w*rows;
    if(n>output_capacity) {
        uint32_t *p=realloc(output_px,n*4); if(!p) return; output_px=p; output_capacity=n;
    }
    memset(output_px,0,n*4);
    if(overlay.open) overlay_render(&overlay,&settings,perf,output_px,w,h);
    else overlay_performance_strip(perf,output_px,w,rows,scale);
    /* Untouched pixels: the menu dims the frozen picture, the performance line is clear. */
    uint32_t clear=overlay.open?0xC0000000u:0;
    for(size_t i=0;i<n;i++) output_px[i]=output_px[i]?0xFF000000u|output_px[i]:clear;
    sdl_host_show(src,sw,sh,aw,ah,output_px,w,rows);
}
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
        /* Queued copies outlive the next publication: they own a reference to their
         * native screen assets until the presenter has taken its own (enh_screens.h). */
        if (qcount==FRAME_QUEUE) { enh_screen_view_release(&queue[qhead].screen_view); qhead=(qhead+1)%FRAME_QUEUE; qcount--; }
        queue[(qhead+qcount)%FRAME_QUEUE]=frame;
        enh_screen_view_retain(&queue[(qhead+qcount++)%FRAME_QUEUE].screen_view);
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

/* --shots DIR: the presented frame every 0.5 s (--shot-interval) of logical time as DIR/NNNN_<step>.bmp. */
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
 *   attract   no key from the front end on: the attract demo (prints the demo player's X)
 *   cave      level 1's cave record 0836h and, 3 s later, its way back 2E7Eh: the player is
 *             held on each source tile until the trigger takes it; prints the simulated
 *             view width after each arrival (true widescreen: 320 in the cave, wide again
 *             back in the main region) */
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

static void scenario_cave(uint64_t now)
{
    static const uint16_t source[2] = { 0x0836, 0x2E7E };
    static unsigned stage; static int placed; static uint64_t arrived;
    if (stage >= 2 || (arrived && now - arrived < 3000000000ull)) return;
    unsigned col = DW(DS_PlayerX) >> 4, scol = source[stage] & 255, srow = source[stage] >> 8;
    if (placed && (col > scol + 20 || col + 20 < scol)) {
        printf("cave: record %04X took the player to tile %02X,%02X at %.2f s: view %u px, camera column %u\n",
               source[stage], col, DW(DS_PlayerY) >> 4, secs(now), pre2_view.width_px, DW(DS_CameraCol));
        stage++; placed = 0; arrived = now; return;
    }
    SET_DW(DS_PlayerX, (uint16_t)(scol * 16 + 8)); SET_DW(DS_PlayerY, (uint16_t)(srow * 16 + 4));
    SET_DB(DS_LandingBlockTimer, 4); SET_DB(DS_PlayerAltModeFlag, 0);
    SET_DW(DS_PlayerVelX, 0); SET_DW(DS_PlayerVelY, 0);
    placed = 1;
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
    if (scenario_poked && !strcmp(scenario, "cave")) { scenario_cave(now); return; }
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
    if (draws) printf("smoke: presenter: %llu images, %.2f ms each\n", (unsigned long long)draws, draw_ns / 1e6 / draws);
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

static uint64_t next_shot_ns, shot_interval_ns = 500000000ull;
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

typedef struct WorkerConfig { RtConfig machine; int turbo; } WorkerConfig;
static int simulation(void *arg)
{
    WorkerConfig *cfg=arg;
    int status=0;
    RtStatus st=RT_STEP_DONE;
    uint64_t bound=0;
    if (rt_init(&cfg->machine)) { fprintf(stderr,"rt_init failed\n"); status=1; goto done; }
    for (;;) {
        int paused=0;
        if (threaded) {
            uint8_t pending[sizeof keys]; unsigned n; int quit;
            sdl_host_lock();
            paused=host_clock.paused; worker_paused=paused; quit=want_quit;
            n=paused?0:key_count; memcpy(pending,keys,n); if(!paused) key_count=0;
            if(!paused) bound=cfg->turbo?bound+1000000000ull/60:host_clock_bound(&host_clock,sdl_host_ns());
            sdl_host_unlock();
            for (unsigned i=0;i<n;i++) rt_key(pending[i]);
            if (quit) { rt_request_quit(); st=RT_QUIT; break; }
            if(paused) {sdl_host_sleep_ns(1000000); continue;}
        }
        else bound=cfg->turbo?bound+1000000000ull/60:host_clock_bound(&host_clock,sdl_host_ns());
        if (smoke_ns && bound>smoke_ns) bound=smoke_ns;
        st=rt_run_until(bound);
        if (threaded) {
            sdl_host_lock(); published_bound=bound; published_now=rt_now(); published_ticks=rt_stats()->ticks; sdl_host_unlock();
        }
        else {
            PresenterImage im={0}; im.pixels=frame_px; im.capacity=sizeof frame_px/sizeof *frame_px;
            uint64_t draw0=sdl_host_ns();
            int drawn=!presenter_render(presenter,bound,1.6,&im);
            draw_ns+=sdl_host_ns()-draw0; draws++;
            if (drawn) {
                frame_w=im.w; frame_h=im.h; shown_screen=frame.screen;
                if (shots_dir && bound>=next_shot_ns) { shot(); next_shot_ns=bound+shot_interval_ns; }
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
    const char *assets = NULL;
    static char found_assets[1024];
    console_attach();
    double smoke = 0;
    int year_override = 0, turbo = 0, headless = 0;
    RtConfig cfg = { 0 };
    for(int i=1;i<argc;i++) if(!strcmp(argv[i],"--settings") && i+1<argc) {
        if(strlen(argv[i+1])>=sizeof settings_path) {fprintf(stderr,"settings path too long\n"); return 1;}
        strcpy(settings_path,argv[++i]);
    }
    if(!*settings_path && sdl_host_settings_path(settings_path,sizeof settings_path)) {
        *settings_path=0;
        fprintf(stderr,"settings: preferences unavailable; continuing with defaults (use --settings PATH)\n");
    }
    saved_settings=settings_defaults();
    if(*settings_path) settings_load(&saved_settings,settings_path,stderr);
    settings=saved_settings;
    printf("settings: %s\n",*settings_path?settings_path:"unavailable (run-only)");
    cfg.audio_native = 1;                     /* native audio engine; --reference: the SB path */
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--assets") && i + 1 < argc) assets = argv[++i];
        else if (!strcmp(argv[i], "--smoke") && i + 1 < argc) smoke = atof(argv[++i]);
        else if (!strcmp(argv[i], "--shots") && i + 1 < argc) shots_dir = argv[++i];
        else if (!strcmp(argv[i], "--shot-interval") && i + 1 < argc) shot_interval_ns = (uint64_t)(atof(argv[++i]) * 1e9);
        else if (!strcmp(argv[i], "--fast-load")) cfg.fast_load = 1;
        else if (!strcmp(argv[i], "--scenario") && i + 1 < argc) scenario = argv[++i];
        else if (!strcmp(argv[i], "--fast-vga")) cfg.fast_vga = 1;
        else if (!strcmp(argv[i], "--reference")) { reference_video = 1; cfg.audio_native = 0; }
        else if (!strcmp(argv[i], "--turbo")) turbo = 1;
        else if (!strcmp(argv[i], "--headless")) headless = turbo = 1;
        else if (!strcmp(argv[i], "--demo-out") && i + 1 < argc) demo_out = argv[++i];
        else if (!strcmp(argv[i], "--demo-in") && i + 1 < argc) demo_in = argv[++i];
        else if (!strcmp(argv[i], "--year") && i + 1 < argc) year_override = atoi(argv[++i]);
        else if (!strcmp(argv[i],"--settings") && i+1<argc) i++;
        else {
            int found=0;
            for(int id=0;id<SET_COUNT;id++) {
                char option[80]; snprintf(option,sizeof option,"--%s",settings_key((Pre2Setting)id));
                for(char *c=option+2;*c;c++) if(*c=='_') *c='-';
                if(strcmp(argv[i],option)) continue;
                const char *v=NULL;
                int boolean=id==SET_INTERPOLATE || id==SET_SMOOTH_CAMERA || id==SET_SMOOTH_TRANSITIONS ||
                    id==SET_FULLSCREEN || id==SET_VSYNC || id==SET_INTEGER_SCALING || id==SET_MUTE || id==SET_PERFORMANCE;
                if(i+1<argc && strncmp(argv[i+1],"--",2)) v=argv[++i];
                else if(boolean) v="true";
                if(!v || settings_set(&settings,(Pre2Setting)id,v)) {
                    fprintf(stderr,"invalid or missing value for %s\n",option); return 1;
                }
                found=1; break;
            }
            if(!found) {fprintf(stderr,"unknown or incomplete option: %s\n",argv[i]); return 1;}
        }
    }
    presentation=settings_presenter(&settings);
    if (settings.wide_sim) {
        unsigned w=(unsigned)settings.wide_sim;
        if(settings.wide_sim==-1) {
            double a=presentation.aspect>0?presentation.aspect:16.0/9;
            w=(unsigned)(200*a/16+.5)*16u; if(w<320)w=320; if(w>640)w=640;
        }
        if(rt_set_wide_sim_width(w)) return 1;
        settings.mode=PRESENT_WIDESCREEN; presentation.mode=PRESENT_WIDESCREEN;
        printf("true widescreen: simulated view %u px wide (fixed for this boot)\n",w);
    }

    if (!assets) {
        if (sdl_host_find_assets(found_assets, sizeof found_assets)) {
            sdl_host_fatal("Prehistorik 2 game files were not found.\n\n"
                           "Put pre2_sdl3.exe and SDL3.dll in the game folder (next to SPRITES.SQZ), "
                           "or start it with --assets DIR.");
            return 1;
        }
        assets = found_assets;
    }
    time_t t = time(NULL); struct tm *lt = localtime(&t);
    rt_boot(assets, year_override ? year_override : lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday);
    if (!reference_video) { vga_memory_off = 1; compositor_init(); }
    if (sdl_host_init(!headless, !turbo)) return 1;
    sdl_host_apply(&settings);
    if (reference_video) presentation=presenter_defaults();
    presenter=presenter_create(&presentation);
    if (!presenter) return 1;
    audio_on = !turbo;
    if (smoke > 0) { smoke_ns = (uint64_t)(smoke * 1e9); smoke_on = 1; }
    rt_on_display_frame = on_display_frame;
    rt_on_pcm = on_pcm;
    rt_on_service = on_service;
    WorkerConfig worker_cfg={cfg,turbo};
    host_clock_init(&host_clock,sdl_host_ns());
    int status;
    threaded=!headless;
    if (!threaded) status=simulation(&worker_cfg);
    else {
        if (sdl_host_worker_start(simulation,&worker_cfg)) return 1;
        uint64_t perf_wall=sdl_host_ns(),perf_ticks=0,perf_frames=0;
        OverlayPerf perf={0};
        for (;;) {
            uint64_t frame_start=sdl_host_ns();
            if (sdl_host_pump(queue_key,ui_key)) { sdl_host_lock(); want_quit=1; sdl_host_unlock(); }
            /* Short copies only under the lock. Every raster and SDL call is
             * outside it; slow presentation can only overflow the frame queue. */
            unsigned available;
            sdl_host_lock(); available=qcount; sdl_host_unlock();
            while (available--) {
                sdl_host_lock();
                incoming=queue[qhead]; qhead=(qhead+1)%FRAME_QUEUE; qcount--;
                sdl_host_unlock();
                presenter_push(presenter,&incoming); remember(&incoming); shown_screen=incoming.screen;
                enh_screen_view_release(&incoming.screen_view);
            }
            uint64_t bound,logical,ticks,mapped; int done; unsigned pending;
            sdl_host_lock(); bound=published_bound; logical=published_now; ticks=published_ticks;
            mapped=turbo?bound:host_clock_target(&host_clock,sdl_host_ns()); done=worker_done; pending=qcount; sdl_host_unlock();
            uint64_t now=sdl_host_ns();
            if(now-perf_wall>=500000000ull) {
                double elapsed=(now-perf_wall)/1e9;
                perf.fps=(frames_shown-perf_frames)/elapsed; perf.tps=(ticks-perf_ticks)/elapsed;
                perf_wall=now; perf_ticks=ticks; perf_frames=frames_shown;
            }
            perf.drift_ms=((double)logical-(double)mapped)/1e6; perf.paused=overlay.open;
            PresenterImage im={0}; im.pixels=frame_px; im.capacity=sizeof frame_px/sizeof *frame_px;
            uint64_t draw0=sdl_host_ns();
            /* The image is sampled at the display's own time on the logical clock, not at the
             * bound the worker last happened to finish (that moves in the worker's steps,
             * which do not line up with refreshes and made interpolated motion uneven).
             * The presenter only ever uses frames already published. */
            uint64_t present=turbo?bound:mapped;
            if (!overlay.open && !presenter_render(presenter,present,sdl_host_window_aspect(),&im)) {
                frame_w=im.w; frame_h=im.h;
                last_present_bound=present;

                if (shots_dir && bound>=next_shot_ns) { shot(); next_shot_ns=bound+shot_interval_ns; }
            }
            uint64_t draw1=sdl_host_ns();
            show_product(&perf); frames_shown++;
            uint64_t show1=sdl_host_ns();
            perf.draw_ms+=((draw1-draw0)/1e6-perf.draw_ms)*.05; perf.show_ms+=((show1-draw1)/1e6-perf.show_ms)*.05;
            if(settings.frame_cap>0) {
                uint64_t interval=1000000000ull/(unsigned)settings.frame_cap,spent=sdl_host_ns()-frame_start;
                if(spent<interval) sdl_host_sleep_ns(interval-spent);
            }
            if (done && !pending) break;
        }
        status=sdl_host_worker_join();
        if (smoke_on) printf("host: last presented %.1f images/s; draw %.2f ms, show %.2f ms (includes the wait for sync)\n",
                             perf.fps, perf.draw_ms, perf.show_ms);
    }
    if (smoke_on) smoke_report();
    if (demo_out) demo_save();
    presenter_destroy(presenter);
    free(output_px);
    sdl_host_shutdown();
    return status;
}
