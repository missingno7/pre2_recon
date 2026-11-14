/* The logical machine (see machine.h): the game on a coroutine, interrupts at its service
 * points in logical-time order, and the device state the game's port traffic programs.
 *
 * Devices modelled here are the ones whose port traffic carries timing: the PIT channel 0
 * divisor (timer rate), the PIC mask (whether the audio IRQ is delivered), the keyboard data
 * port, and the reference audio block clock of the Sound Blaster path (DSP 14h block length
 * and time constant 40h; DMA channel 1 address/page/count to find the block's bytes). That
 * last part is the *reference* audio path (the game's own mixer output, block by block); it
 * is replaced by the native audio engine's block clock (native/audio.c) in production.
 */
#include "machine.h"
#include "audio.h"
#include <stdlib.h>
#include "../c/pre2.h"
#include "../c/vga.h"
#include "../c/ports.h"
#include "../c/platform.h"
#include "../c/system.h"
#include "../c/view.h"
#include "level_regions.h"
#include "../c/interrupts.h"
#include <string.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <ucontext.h>
#include <stdlib.h>
#endif

int ProgramEntry(void);

void (*rt_on_display_frame)(uint64_t t_ns, uint16_t start_address) = 0;
void (*rt_on_pcm)(uint64_t t_ns, const uint8_t *pcm, uint32_t n, uint32_t rate) = 0;
void (*rt_on_service)(void) = 0;

#define NEVER UINT64_MAX

static Audio *audio;
void (*rt_on_audio_command)(uint64_t, const AudioCmd *) = 0;
static RtConfig cfg;
static unsigned wide_sim_width;
static RtStats stats;

/* ---- logical time ------------------------------------------------------------------------- */
static uint64_t now;            /* logical ns */
static uint64_t target;         /* the host's bound for this run */
static int quit, exited, exit_code;

/* ---- coroutine ---------------------------------------------------------------------------- */
#ifdef _WIN32
static LPVOID host_fiber, game_fiber;
static void switch_to_host(void) { SwitchToFiber(host_fiber); }
static void switch_to_game(void) { SwitchToFiber(game_fiber); }
#else
static ucontext_t host_ctx, game_ctx;
static void switch_to_host(void) { swapcontext(&game_ctx, &host_ctx); }
static void switch_to_game(void) { swapcontext(&host_ctx, &game_ctx); }
#endif

/* Suspend the game while its logical time is at or past the host's bound. */
static void yield_if_due(void)
{
    while (!quit && now >= target) switch_to_host();
    if (quit) for (;;) switch_to_host();       /* never resumed after a quit */
}

/* ---- PIT channel 0 ------------------------------------------------------------------------ */
static struct {
    uint16_t divisor; int hi; uint8_t latch;
    uint64_t next_ns; uint64_t next_rem;   /* next tick at next_ns + next_rem / RT_PIT_HZ ns */
} pit;

static void pit_schedule_next(void)
{
    uint64_t d = pit.divisor ? pit.divisor : 65536u;
    uint64_t num = d * RT_NS_PER_SEC;
    pit.next_ns += num / RT_PIT_HZ;
    pit.next_rem += num % RT_PIT_HZ;
    if (pit.next_rem >= RT_PIT_HZ) { pit.next_ns++; pit.next_rem -= RT_PIT_HZ; }
}

static uint16_t vector_off(int n) { return rd16(0, (uint16_t)(4 * n)); }

static void run_int08(void)
{
    uint16_t off = vector_off(8);
    if (off == CS_SpeakerTimerIsr) SpeakerTimerIsr();
    else if (off == CS_SampleTimerIsr) SampleTimerIsr();
    stats.ticks++;
}

/* ---- keyboard ----------------------------------------------------------------------------- */
#define KEYQ 256
static struct { uint8_t code[KEYQ]; uint64_t t[KEYQ]; unsigned head, tail; uint8_t port60; } kb;

void rt_key_at(uint8_t code, uint64_t t)
{
    if (kb.tail - kb.head >= KEYQ) return;                 /* overrun: the controller drops it */
    if (kb.head != kb.tail && t < kb.t[(kb.tail - 1) % KEYQ]) t = kb.t[(kb.tail - 1) % KEYQ];
    kb.code[kb.tail % KEYQ] = code;
    kb.t[kb.tail % KEYQ] = t;
    kb.tail++;
}
void rt_key(uint8_t code) { rt_key_at(code, target > now ? target : now); }

static void run_int09(uint8_t code)
{
    if (vector_off(9) != CS_KeyboardIsr) return;
    kb.port60 = code;
    if (KeyboardIsr() == SYS_QUIT) quit = 1;
}

/* ---- reference audio block clock (Sound Blaster DSP 14h, DMA channel 1, PIC mask) ---------- */
static struct {
    uint8_t dma_flipflop, dma_page; uint16_t dma_addr, dma_count;
    int dsp_cmd, dsp_args; uint8_t dsp_arg[2];
    uint32_t sample_us;         /* 256 - time constant: one sample's length in us */
    uint64_t block_end;         /* NEVER when no block is playing */
    uint8_t pic_mask;
    uint64_t irq_due;           /* while the block IRQ runs: when the block ended */
    int in_irq;
} sb = { 0, 0, 0, 0, -1, 0, {0, 0}, 119, NEVER, 0, 0, 0 };

static void sb_block_start(void)
{
    /* Reference convention (docs/native.md): a block started by the IRQ of the one before
     * starts when that one ended. Single-cycle DSP 14h output really restarts only when the
     * handler reprograms it, a few microseconds after a timely IRQ; the machine delivers
     * interrupts at service points, which can be milliseconds late during drawing, so the
     * block clock is kept at the nominal 19992 us period instead of drifting with that
     * delivery delay. The handler's effects on memory still happen at delivery. */
    uint64_t start = sb.in_irq ? sb.irq_due : now;
    uint32_t lin = ((uint32_t)sb.dma_page << 16) + sb.dma_addr;
    uint32_t n = (uint32_t)sb.dma_count + 1;
    if (lin + n <= MEM_SIZE) {
        if (rt_on_pcm) rt_on_pcm(start, pre2_mem + lin, n, 1000000u / sb.sample_us);
        stats.pcm_bytes += n;
    }
    sb.block_end = start + (uint64_t)n * sb.sample_us * 1000u;
}

static void run_sb_irq(void)
{
    stats.audio_blocks++;
    int irq = DB(DS_SbIrqLine);
    if (irq < 2 || irq > 7 || vector_off(8 + irq) != CS_SbIrqHandler) return;
    if (sb.pic_mask & (1 << irq)) return;
    SbIrqHandler(0x01F0);
}

static void native_pcm(void *user, uint64_t t, const uint8_t *pcm)
{
    (void)user;
    if (rt_on_pcm) rt_on_pcm(t,pcm,AUDIO_BLOCK_BYTES,1000000u/119);
    stats.pcm_bytes += AUDIO_BLOCK_BYTES;
}
static void audio_sink(const AudioCmd *cmd)
{
    if (rt_on_audio_command) rt_on_audio_command(now,cmd);
    if (cfg.audio_native && audio_command(audio,cmd,now)) {
        fprintf(stderr,"native audio: invalid command %u/%u\n",cmd->kind,cmd->id);
        abort();
    }
}

static uint64_t audio_next(void) { return cfg.audio_native ? audio_deadline(audio) : sb.block_end; }

/* ---- device ports ------------------------------------------------------------------------- */
static uint16_t sb_base(void) { return CW(CS_SbBasePort); }

static void dev_out(uint16_t port, uint8_t v)
{
    uint16_t b = sb_base();
    if (port == 0x40) {
        if (!pit.hi) { pit.latch = v; pit.hi = 1; }
        else { pit.divisor = (uint16_t)(pit.latch | v << 8); pit.hi = 0; }
    }
    else if (port == 0x43) pit.hi = 0;
    else if (cfg.audio_native) return; /* SB/DMA/PIC output is reference-only. */
    else if (port == 0x0C) sb.dma_flipflop = 0;
    else if (port == 0x02) { if (!sb.dma_flipflop) sb.dma_addr = (uint16_t)((sb.dma_addr & 0xFF00) | v); else sb.dma_addr = (uint16_t)((sb.dma_addr & 0xFF) | v << 8); sb.dma_flipflop ^= 1; }
    else if (port == 0x03) { if (!sb.dma_flipflop) sb.dma_count = (uint16_t)((sb.dma_count & 0xFF00) | v); else sb.dma_count = (uint16_t)((sb.dma_count & 0xFF) | v << 8); sb.dma_flipflop ^= 1; }
    else if (port == 0x83) sb.dma_page = v;
    else if (port == 0x21) sb.pic_mask = v;
    else if (b && port == (uint16_t)(b + 0x0C)) {          /* DSP write: command + arguments */
        if (sb.dsp_cmd < 0) {
            sb.dsp_cmd = v; sb.dsp_args = 0;
            if (v != 0x14 && v != 0x40) sb.dsp_cmd = -1;   /* the commands with arguments we model */
        } else {
            sb.dsp_arg[sb.dsp_args++] = v;
            if (sb.dsp_cmd == 0x40) { sb.sample_us = 256u - v; if (!sb.sample_us) sb.sample_us = 1; sb.dsp_cmd = -1; }
            else if (sb.dsp_cmd == 0x14 && sb.dsp_args == 2) { sb.dsp_cmd = -1; sb_block_start(); }
        }
    }
}

static uint8_t dev_in(uint16_t port)
{
    uint16_t b = sb_base();
    if (port == 0x60) return kb.port60;
    if (port == 0x201) return 0xFF;                        /* no joystick */
    if (port == 0x21) return sb.pic_mask;
    if (b && port == (uint16_t)(b + 0x0C)) return 0x00;    /* DSP ready for writes */
    if (b && port == (uint16_t)(b + 0x0E)) return 0xFF;    /* data available / IRQ ack */
    if (b && port == (uint16_t)(b + 0x0A)) return 0xAA;
    return 0x00;
}

static PortDevice device = { dev_out, dev_in };

/* ---- CRT timeline (timing only: when retrace starts and each frame is displayed) -----------
 * Line time = horizontal total (CRTC 00h + 5 character clocks of 8 or 9 dots) at the dot
 * clock (misc output bits 2-3, halved by sequencer 01h bit 3); vertical total, display end,
 * retrace start/end from CRTC 06h/07h/10h/11h/12h. Every mode the game sets gives 31.78 us
 * lines and 449 lines (70.1 Hz). The start address is latched at the retrace start and shown
 * from the next frame's line 0. */
typedef struct { uint64_t line_ps; int total, vde, vrs, vre; uint32_t hde_permille; } CrtGeom;
static CrtGeom crt_geom(void)
{
    const VgaState *v = &pre2_vga;
    CrtGeom g;
    uint64_t dots = (v->seq[1] & 1) ? 8 : 9;
    uint64_t clock = ((v->misc_output >> 2) & 3) == 1 ? 28322000u : 25175000u;
    if (v->seq[1] & 0x08) clock /= 2;
    uint64_t ht = (uint64_t)v->crtc[0] + 5;
    g.line_ps = ht * dots * 1000000000000ull / clock;
    g.total = (v->crtc[6] | ((v->crtc[7] & 0x01) << 8) | ((v->crtc[7] & 0x20) << 4)) + 2;
    g.vde = (v->crtc[0x12] | ((v->crtc[7] & 0x02) << 7) | ((v->crtc[7] & 0x40) << 3)) + 1;
    g.vrs = v->crtc[0x10] | ((v->crtc[7] & 0x04) << 6) | ((v->crtc[7] & 0x80) << 2);
    int len = ((v->crtc[0x11] & 0x0F) - g.vrs) & 0x0F;
    g.vre = g.vrs + (len ? len : 16);
    g.hde_permille = (uint32_t)(((uint64_t)v->crtc[1] + 1) * 1000u / ht);
    if (g.line_ps < 10000000ull || g.line_ps > 100000000ull || g.total < 200 || g.total > 1100 || g.vde >= g.total ||
        g.vrs >= g.total) { g.line_ps = 31777557ull; g.total = 449; g.vde = 400; g.vrs = 412; g.vre = 414; g.hde_permille = 800; }
    return g;
}
static uint64_t lines_ns(const CrtGeom *g, int n) { return (uint64_t)n * g->line_ps / 1000u; }

static struct {
    uint64_t frame_ns;      /* line 0 of the current frame */
    int latched;
    uint16_t start, next_start;
    int started;
} crt;

static uint16_t crtc_start(void) { return (uint16_t)(pre2_vga.crtc[0x0C] << 8 | pre2_vga.crtc[0x0D]); }

static uint64_t crt_next_edge(void)
{
    CrtGeom g = crt_geom();
    uint64_t retrace = crt.frame_ns + lines_ns(&g, g.vrs);
    return crt.latched ? crt.frame_ns + lines_ns(&g, g.total) : retrace;
}

static void crt_advance(void)
{
    if (!crt.started) { crt.started = 1; crt.frame_ns = now; }
    for (;;) {
        CrtGeom g = crt_geom();
        uint64_t retrace = crt.frame_ns + lines_ns(&g, g.vrs);
        if (!crt.latched && now >= retrace) { crt.next_start = crtc_start(); crt.latched = 1; }
        uint64_t next = crt.frame_ns + lines_ns(&g, g.total);
        if (now < next) break;
        crt.frame_ns = next;
        if (!crt.latched) crt.next_start = crtc_start();
        crt.latched = 0;
        crt.start = crt.next_start;
        stats.display_frames++;
        if (rt_on_display_frame) rt_on_display_frame(crt.frame_ns, crt.start);
    }
}

/* 3DAh: bit 3 vertical retrace, bit 0 display inactive, from the scan position at `now`. */
static uint8_t crt_status(void)
{
    CrtGeom g = crt_geom();
    uint64_t into = (now - crt.frame_ns) * 1000u;          /* ps since line 0 */
    uint64_t line = into / g.line_ps;
    uint64_t frac = (into % g.line_ps) * 1000u / g.line_ps;
    int retrace = (int)line >= g.vrs && (int)line < g.vre;
    int inactive = (int)line >= g.vde || frac >= g.hde_permille;
    return (uint8_t)((retrace ? 0x08 : 0x00) | (inactive ? 0x01 : 0x00));
}

/* ---- bus time ----------------------------------------------------------------------------- */
static uint32_t paid_mem, paid_io;
static void bus_advance(void)
{
    uint32_t m = vga_mem_accesses, io = vga_io_accesses;
    uint64_t d = (uint64_t)(uint32_t)(io - paid_io) * 1000u;
    if (!cfg.fast_vga) d += (uint64_t)(uint32_t)(m - paid_mem) * 500u;
    paid_mem = m; paid_io = io;
    now += d; stats.bus_ns += d;
}

/* ---- event delivery ----------------------------------------------------------------------- */
static uint64_t next_event(void)
{
    uint64_t t = pit.next_ns;
    if (audio_next() < t) t = audio_next();
    uint64_t c = crt_next_edge();
    if (c < t) t = c;
    if (kb.head != kb.tail && kb.t[kb.head % KEYQ] < t) t = kb.t[kb.head % KEYQ];
    return t;
}

/* Every event due at `now`, in time order; simultaneous interrupts in the master PIC's
 * priority order: IRQ0 timer, IRQ1 keyboard, IRQ7 audio (CRT edges are not interrupts). */
static void deliver_due(void)
{
    for (;;) {
        crt_advance();
        uint64_t tk = kb.head != kb.tail ? kb.t[kb.head % KEYQ] : NEVER;
        uint64_t best = pit.next_ns;
        int which = 1;
        if (tk < best) { best = tk; which = 0; }
        if (audio_next() < best) { best = audio_next(); which = 2; }
        if (best > now) break;
        if (which == 0) { uint8_t c = kb.code[kb.head % KEYQ]; kb.head++; run_int09(c); }
        else if (which == 1) { run_int08(); pit_schedule_next(); }
        else if (cfg.audio_native) { stats.audio_blocks++; audio_complete(audio); }
        else { sb.irq_due = sb.block_end; sb.block_end = NEVER; sb.in_irq = 1; run_sb_irq(); sb.in_irq = 0; }
        if (quit) break;
    }
}

/* Jump to the next event (bounded by the host's target: suspend first). */
static void wait_next_event(void)
{
    for (;;) {
        uint64_t t = next_event();
        if (t <= now) break;
        if (t <= target) { now = t; break; }
        now = target;            /* nothing happens until the bound: suspend there */
        yield_if_due();
    }
}

static void service(int waiting)
{
    bus_advance();
    deliver_due();
    if (rt_on_service) rt_on_service();
    if (waiting) { stats.tick_waits++; wait_next_event(); }
    else { stats.polls++; now += RT_POLL_TURN_NS; }
    deliver_due();
    yield_if_due();
}

static uint8_t status_3da(void)
{
    bus_advance();
    deliver_due();
    yield_if_due();
    return crt_status();
}

static void access_hook(void) { bus_advance(); crt_advance(); }

static void load_time(uint32_t opens, uint32_t bytes)
{
    if (cfg.fast_load) return;
    uint64_t d = opens * 30000000ull + bytes * 10000ull;
    uint64_t end = now + d;
    stats.load_ns += d;
    while (now < end) {
        uint64_t t = next_event();
        now = t < end ? t : end;
        deliver_due();
        if (rt_on_service) rt_on_service();
        yield_if_due();
    }
}

/* ---- the game coroutine --------------------------------------------------------------------- */
#ifdef _WIN32
static void WINAPI game_main(LPVOID p)
#else
static void game_main(void)
#endif
{
#ifdef _WIN32
    (void)p;
#endif
    exit_code = ProgramEntry();
    exited = 1;
    for (;;) switch_to_host();
}

int rt_set_wide_sim_width(unsigned width)
{
    if (width && (width<320 || width>640 || width%16)) return -1;
    wide_sim_width=width;
    return 0;
}

int rt_init(const RtConfig *c)
{
    memset(&cfg,0,sizeof cfg);
    if (c) cfg = *c;
    pre2_view_region_policy = pre2_region_policy;
    if (pre2_view_configure(wide_sim_width)) return 1;
    /* Wide semantic margin work has no emulated bus cost. The original fixed
     * raster operations still charge their original accesses; see native-widescreen.md. */
    audio_destroy(audio); audio=NULL;
    if (cfg.audio_native && !(audio=audio_create(native_pcm,NULL))) return 1;
    pre2_audio_sink=audio_sink;
    now = 0; target = 0; quit = exited = exit_code = 0;
    memset(&stats, 0, sizeof stats);
    memset(&pit, 0, sizeof pit);
    memset(&crt, 0, sizeof crt);
    memset(&kb, 0, sizeof kb);
    sb.dma_flipflop = sb.dma_page = 0; sb.dma_addr = sb.dma_count = 0; sb.dsp_cmd = -1; sb.dsp_args = 0;
    sb.sample_us = 119; sb.block_end = NEVER; sb.pic_mask = 0; sb.in_irq = 0;
    paid_mem = vga_mem_accesses; paid_io = vga_io_accesses;
    ports_install(&device);
    vga_status_hook = status_3da;
    vga_access_hook = access_hook;
    pre2_host_poll = service;
    pre2_host_io = load_time;
#ifdef _WIN32
    if (!host_fiber) host_fiber = ConvertThreadToFiber(NULL);   /* once per process */
    if (game_fiber) DeleteFiber(game_fiber);
    game_fiber = CreateFiber(8u << 20, game_main, NULL);
    if (!game_fiber) return 1;
#else
    static char *stack;
    free(stack);
    stack = malloc(8u << 20);
    getcontext(&game_ctx);
    game_ctx.uc_stack.ss_sp = stack; game_ctx.uc_stack.ss_size = 8u << 20; game_ctx.uc_link = &host_ctx;
    makecontext(&game_ctx, game_main, 0);
#endif
    return 0;
}

RtStatus rt_run_until(uint64_t t)
{
    if (exited) return RT_EXITED;
    if (quit) return RT_QUIT;
    if (t <= now) return RT_STEP_DONE;
    target = t;
    switch_to_game();
    if (exited) return RT_EXITED;
    if (quit) return RT_QUIT;
    return RT_STEP_DONE;
}

uint64_t rt_now(void) { return now; }
int rt_exit_code(void) { return exit_code; }
void rt_request_quit(void) { quit = 1; }
const RtStats *rt_stats(void) { return &stats; }
