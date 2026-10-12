/* Native platform: the machine around the translated game, on SDL3.
 *
 * The game runs on the main thread inside ProgramEntry(). It never yields except where the
 * original busy-waits: the frame tick wait (platform_tick_wait), other loops
 * (platform_poll) and the VGA status register 3DAh (vga_status_hook). Those are the points
 * where this host services the machine:
 *   - timer: PIT channel 0 ticks at 1193182 / divisor Hz of real time (the divisor the game
 *     programs through port 40h); each due tick runs the INT 08h handler the game has
 *     installed in the interrupt vector table (SpeakerTimerIsr or SampleTimerIsr).
 *   - keyboard: SDL key events become PC scan codes (set 1, make/break); each runs the
 *     installed INT 09h handler (KeyboardIsr) with the code latched on port 60h.
 *   - Sound Blaster: the game's DSP 14h command starts an 8-bit DMA block of channel 1; the
 *     block's bytes (at the DMA page/address the game programmed) go to the SDL audio
 *     stream, and the block's IRQ (SbIrqHandler) is raised when the stream has drained to
 *     one queued block, so mixing keeps pace with playback.
 *   - video: a CRT timeline from the CRTC timing registers and real time (crt_advance):
 *     3DAh reports vertical retrace and display-inactive as the scan position says, the
 *     start address is latched at the start of vertical retrace (as the CRTC does: a
 *     program writes the new start before the retrace and the pel panning during it), and
 *     each frame is scanned out from the model when its display starts. The timeline is
 *     advanced before every VGA register access (vga_access_hook) and at service points,
 *     so register writes take effect in time order.
 *   - loading: each DOS file open and read takes the time it took on a period machine
 *     (host_io below), and the machine is serviced meanwhile. Without it a load is
 *     instantaneous, and screens that test a held key on their first frame (the MODE
 *     screen and the map scroll-in exit while RETURN is down) see the key that started
 *     them still held and end before they are shown.
 * Interrupts are delivered only at these points: a timing the original hardware can also
 * produce (docs/timing.md).
 */
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../c/pre2.h"
#include "../c/vga.h"
#include "../c/ports.h"
#include "../c/platform.h"
#include "../c/system.h"
#include "../c/interrupts.h"
#include "present.h"
#include "../c/game.h"
#include "platform_host.h"
#ifdef _WIN32
#include <windows.h>
#endif

static SDL_Window *win;
static uint64_t last_service_ns;   /* watchdog: last service point */
static SDL_Renderer *ren;
static SDL_Texture *tex;
static SDL_AudioStream *audio;
static uint32_t pixels[PRESENT_MAX_W * PRESENT_MAX_H];
static int tex_w = 320, tex_h = 200;

/* ---- machine state outside the game's memory (device registers) ------------------------ */
static struct {
    uint16_t pit_divisor; int pit_hi; uint16_t pit_latch;
    uint8_t dma_flipflop; uint16_t dma_addr, dma_count; uint8_t dma_page;
    int dsp_cmd, dsp_args; uint8_t dsp_arg[2]; int sb_rate;
    int block_pending;      /* a started DMA block whose completion IRQ has not run yet */
    uint8_t pic_mask;
    uint8_t scancode;
    uint64_t next_tick_ns;
    int quit;
} hw = { 0, 0, 0, 0, 0, 0, 0, -1, 0, {0, 0}, 8403, 0, 0, 0, 0, 0 };

static uint16_t sb_base(void) { return CW(CS_SbBasePort); }

static void pit_out(uint8_t v)
{
    if (!hw.pit_hi) { hw.pit_latch = v; hw.pit_hi = 1; }
    else { hw.pit_divisor = (uint16_t)(hw.pit_latch | v << 8); hw.pit_hi = 0; }
}

static uint64_t tick_period_ns(void)
{
    uint32_t d = hw.pit_divisor ? hw.pit_divisor : 65536u;
    return (uint64_t)d * 1000000000ull / 1193182ull;
}

static void sb_block(void)
{
    uint32_t lin = ((uint32_t)hw.dma_page << 16) + hw.dma_addr;
    uint32_t n = (uint32_t)hw.dma_count + 1;
    if (audio && lin + n <= MEM_SIZE) { SDL_PutAudioStreamData(audio, pre2_mem + lin, (int)n); host_stats.sb_bytes += n; }
    hw.block_pending = 1;
}

static void dev_out(uint16_t port, uint8_t v)
{
    uint16_t b = sb_base();
    if (port == 0x40) pit_out(v);
    else if (port == 0x43) hw.pit_hi = 0;
    else if (port == 0x0C) hw.dma_flipflop = 0;
    else if (port == 0x02) { if (!hw.dma_flipflop) hw.dma_addr = (uint16_t)((hw.dma_addr & 0xFF00) | v); else hw.dma_addr = (uint16_t)((hw.dma_addr & 0xFF) | v << 8); hw.dma_flipflop ^= 1; }
    else if (port == 0x03) { if (!hw.dma_flipflop) hw.dma_count = (uint16_t)((hw.dma_count & 0xFF00) | v); else hw.dma_count = (uint16_t)((hw.dma_count & 0xFF) | v << 8); hw.dma_flipflop ^= 1; }
    else if (port == 0x83) hw.dma_page = v;
    else if (port == 0x21) hw.pic_mask = v;
    else if (b && port == (uint16_t)(b + 0x0C)) {          /* DSP write: command + arguments */
        if (hw.dsp_cmd < 0) {
            hw.dsp_cmd = v; hw.dsp_args = 0;
            if (v != 0x14 && v != 0x40) hw.dsp_cmd = -1;   /* commands without arguments we model */
        } else {
            hw.dsp_arg[hw.dsp_args++] = v;
            if (hw.dsp_cmd == 0x40) { hw.sb_rate = 1000000 / (256 - v); hw.dsp_cmd = -1; }
            else if (hw.dsp_cmd == 0x14 && hw.dsp_args == 2) { hw.dsp_cmd = -1; sb_block(); }
        }
    }
}

static uint8_t dev_in(uint16_t port)
{
    uint16_t b = sb_base();
    if (port == 0x60) return hw.scancode;
    if (port == 0x201) return 0xFF;                        /* no joystick */
    if (port == 0x21) return hw.pic_mask;
    if (b && port == (uint16_t)(b + 0x0C)) return 0x00;    /* DSP ready for writes */
    if (b && port == (uint16_t)(b + 0x0E)) return 0xFF;    /* data available / IRQ ack */
    if (b && port == (uint16_t)(b + 0x0A)) return 0xAA;
    return 0x00;
}

static PortDevice device = { dev_out, dev_in };

/* ---- interrupts through the game's vector table ------------------------------------------ */
static uint16_t vector_off(int n) { return rd16(0, (uint16_t)(4 * n)); }

static void run_int08(void)
{
    uint16_t off = vector_off(8);
    if (off == CS_SpeakerTimerIsr) SpeakerTimerIsr();
    else if (off == CS_SampleTimerIsr) SampleTimerIsr();
}

static void run_int09(uint8_t code)
{
    if (vector_off(9) != CS_KeyboardIsr) return;
    hw.scancode = code;
    if (KeyboardIsr() == SYS_QUIT) hw.quit = 1;
}

static void run_sb_irq(void)
{
    int irq = DB(DS_SbIrqLine);
    if (irq < 2 || irq > 7 || vector_off(8 + irq) != CS_SbIrqHandler) return;
    if (hw.pic_mask & (1 << irq)) return;
    SbIrqHandler(0x01F0);
    host_stats.sb_irqs++;
}

/* ---- keyboard: SDL scancode -> PC scan code set 1 ----------------------------------------- */
static uint8_t pc_scancode(SDL_Scancode s, int *extended)
{
    *extended = 0;
    switch (s) {
    case SDL_SCANCODE_ESCAPE: return 0x01;
    case SDL_SCANCODE_1: return 0x02; case SDL_SCANCODE_2: return 0x03; case SDL_SCANCODE_3: return 0x04;
    case SDL_SCANCODE_4: return 0x05; case SDL_SCANCODE_5: return 0x06; case SDL_SCANCODE_6: return 0x07;
    case SDL_SCANCODE_7: return 0x08; case SDL_SCANCODE_8: return 0x09; case SDL_SCANCODE_9: return 0x0A;
    case SDL_SCANCODE_0: return 0x0B; case SDL_SCANCODE_MINUS: return 0x0C; case SDL_SCANCODE_EQUALS: return 0x0D;
    case SDL_SCANCODE_BACKSPACE: return 0x0E; case SDL_SCANCODE_TAB: return 0x0F;
    case SDL_SCANCODE_Q: return 0x10; case SDL_SCANCODE_W: return 0x11; case SDL_SCANCODE_E: return 0x12;
    case SDL_SCANCODE_R: return 0x13; case SDL_SCANCODE_T: return 0x14; case SDL_SCANCODE_Y: return 0x15;
    case SDL_SCANCODE_U: return 0x16; case SDL_SCANCODE_I: return 0x17; case SDL_SCANCODE_O: return 0x18;
    case SDL_SCANCODE_P: return 0x19; case SDL_SCANCODE_RETURN: return 0x1C; case SDL_SCANCODE_LCTRL: return 0x1D;
    case SDL_SCANCODE_A: return 0x1E; case SDL_SCANCODE_S: return 0x1F; case SDL_SCANCODE_D: return 0x20;
    case SDL_SCANCODE_F: return 0x21; case SDL_SCANCODE_G: return 0x22; case SDL_SCANCODE_H: return 0x23;
    case SDL_SCANCODE_J: return 0x24; case SDL_SCANCODE_K: return 0x25; case SDL_SCANCODE_L: return 0x26;
    case SDL_SCANCODE_LSHIFT: return 0x2A; case SDL_SCANCODE_Z: return 0x2C; case SDL_SCANCODE_X: return 0x2D;
    case SDL_SCANCODE_C: return 0x2E; case SDL_SCANCODE_V: return 0x2F; case SDL_SCANCODE_B: return 0x30;
    case SDL_SCANCODE_N: return 0x31; case SDL_SCANCODE_M: return 0x32; case SDL_SCANCODE_RSHIFT: return 0x36;
    case SDL_SCANCODE_LALT: return 0x38; case SDL_SCANCODE_SPACE: return 0x39;
    case SDL_SCANCODE_F1: return 0x3B; case SDL_SCANCODE_F2: return 0x3C; case SDL_SCANCODE_F3: return 0x3D;
    case SDL_SCANCODE_F4: return 0x3E; case SDL_SCANCODE_F5: return 0x3F; case SDL_SCANCODE_F6: return 0x40;
    case SDL_SCANCODE_F7: return 0x41; case SDL_SCANCODE_F8: return 0x42; case SDL_SCANCODE_F9: return 0x43;
    case SDL_SCANCODE_F10: return 0x44;
    case SDL_SCANCODE_UP: return 0x48; case SDL_SCANCODE_LEFT: return 0x4B;
    case SDL_SCANCODE_RIGHT: return 0x4D; case SDL_SCANCODE_DOWN: return 0x50;
    case SDL_SCANCODE_KP_8: return 0x48; case SDL_SCANCODE_KP_4: return 0x4B;
    case SDL_SCANCODE_KP_6: return 0x4D; case SDL_SCANCODE_KP_2: return 0x50;
    default: return 0;
    }
}

/* ---- presentation ------------------------------------------------------------------------- */
/* ---- CRT timeline ------------------------------------------------------------------------ */
/* Scan position from the CRTC registers: line time = horizontal total (CRTC 00h + 5
 * character clocks of 8 or 9 dots, sequencer 01h bit 0) at the dot clock (misc output
 * bits 2-3: 25.175 or 28.322 MHz, halved by sequencer 01h bit 3); vertical total, display
 * end and retrace start/end from CRTC 06h/07h/10h/11h/12h. Line 0 is the first displayed
 * line. All supported modes give 31.78 us lines and 449 lines (70.1 Hz); the vertical
 * retrace lasts 2 lines (~64 us), the vertical blank ~1.6 ms. */
typedef struct { double line_ns; int total, vde, vrs, vre; double hde_frac; } CrtGeom;
static CrtGeom crt_geom(void)
{
    const VgaState *v = &pre2_vga;
    CrtGeom g;
    int dots = (v->seq[1] & 1) ? 8 : 9;
    double clock = ((v->misc_output >> 2) & 3) == 1 ? 28.322e6 : 25.175e6;
    if (v->seq[1] & 0x08) clock /= 2;
    int ht = v->crtc[0] + 5;
    g.line_ns = ht * dots * 1e9 / clock;
    g.total = (v->crtc[6] | ((v->crtc[7] & 0x01) << 8) | ((v->crtc[7] & 0x20) << 4)) + 2;
    g.vde = (v->crtc[0x12] | ((v->crtc[7] & 0x02) << 7) | ((v->crtc[7] & 0x40) << 3)) + 1;
    g.vrs = v->crtc[0x10] | ((v->crtc[7] & 0x04) << 6) | ((v->crtc[7] & 0x80) << 2);
    g.vre = g.vrs + ((((v->crtc[0x11] & 0x0F) - g.vrs) & 0x0F) ? (((v->crtc[0x11] & 0x0F) - g.vrs) & 0x0F) : 16);
    g.hde_frac = (double)(v->crtc[1] + 1) / ht;
    if (g.line_ns < 10000 || g.line_ns > 100000 || g.total < 200 || g.total > 1100 || g.vde >= g.total ||
        g.vrs >= g.total) { g.line_ns = 31777.6; g.total = 449; g.vde = 400; g.vrs = 412; g.vre = 414; g.hde_frac = 0.8; }
    return g;
}

static struct {
    uint64_t frame_ns;      /* time of line 0 of the current frame */
    int latched;            /* the start address of the next frame has been latched */
    uint16_t start;         /* start address latched for the frame being displayed */
    uint16_t next_start;    /* latched at this frame's vertical retrace, shown from the next */
    int frame_ready;        /* a scanned-out frame waits for upload */
} crt;

/* Advance to `now`: latch the start address when the vertical retrace begins, scan the
 * frame out when its display starts (the start latched at the retrace before it). */
static void crt_advance(uint64_t now)
{
    CrtGeom g = crt_geom();
    if (!crt.frame_ns) crt.frame_ns = now;
    uint64_t frame_len = (uint64_t)(g.total * g.line_ns);
    if (now - crt.frame_ns > 4 * frame_len) {             /* far behind (host stalled): resync */
        crt.frame_ns = now - (now - crt.frame_ns) % frame_len;
        crt.latched = 0;
    }
    for (;;) {
        uint64_t retrace = crt.frame_ns + (uint64_t)(g.vrs * g.line_ns);
        if (!crt.latched && now >= retrace) {
            crt.next_start = (uint16_t)(pre2_vga.crtc[0x0C] << 8 | pre2_vga.crtc[0x0D]);
            crt.latched = 1;
        }
        uint64_t next = crt.frame_ns + frame_len;
        if (now < next) break;
        crt.frame_ns = next;
        if (!crt.latched) crt.next_start = (uint16_t)(pre2_vga.crtc[0x0C] << 8 | pre2_vga.crtc[0x0D]);
        crt.latched = 0;
        crt.start = crt.next_start;
        if (now < next + frame_len) {                     /* scan out only the current frame */
            int w, h;
            present_vga_start(pixels, &w, &h, crt.start);
            if (w != tex_w || h != tex_h) crt.frame_ready = 2; else crt.frame_ready = 1;
            tex_w = w; tex_h = h;
        }
    }
}

/* VGA bus time: the VGA sits on the ISA bus, where each CPU access to video memory costs
 * about 0.5 us and each port access about 1 us (16-bit ISA at 8 MHz with the card's wait
 * states). Code that is busy with the VGA therefore took real time on the original, and
 * some loops depend on it: the MODE screen's frame loop waits only for the retrace bit to
 * be set (WaitVerticalRetraceStart), and with CRTC 11h = 24h the retrace lasts 8 lines
 * (254 us); its frame work (two strings, a column and a row copy: thousands of accesses)
 * outlasted that, so it ran once per retrace. Accesses are counted by the model
 * (vga_mem_accesses, vga_io_accesses) and paid here by spinning, in chunks of at least
 * 20 us, before the CRT timeline is advanced. --fast-vga turns it off. */
int host_fast_vga = 0;
static void bus_pay(void)
{
    static uint32_t paid_mem, paid_io;
    static uint64_t debt;
    uint32_t m = vga_mem_accesses, io = vga_io_accesses;
    debt += (uint64_t)(uint32_t)(m - paid_mem) * 500u + (uint64_t)(uint32_t)(io - paid_io) * 1000u;
    paid_mem = m; paid_io = io;
    if (host_fast_vga) { debt = 0; return; }
    if (debt < 20000u) return;
    uint64_t end = SDL_GetTicksNS() + debt;
    debt = 0;
    while (SDL_GetTicksNS() < end) { }
}

static void crt_hook(void) { bus_pay(); crt_advance(SDL_GetTicksNS()); }

static void present(void)
{
    int w = tex_w, h = tex_h;
    if (crt.frame_ready == 2) {                 /* the program changed the video mode */
        SDL_DestroyTexture(tex);
        tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    }
    crt.frame_ready = 0;
    SDL_UpdateTexture(tex, NULL, pixels, w * 4);
    SDL_RenderClear(ren);
    SDL_RenderTexture(ren, tex, NULL, NULL);
    SDL_RenderPresent(ren);
    host_stats.presents++;
}

HostStats host_stats;

/* --shots DIR: the presented frame every 0.5 s as DIR/NNNN_<step>.bmp (diagnostics). */
const char *host_shots_dir = 0;
static uint64_t next_shot_ns;
static void shot(void)
{
    static int n;
    char path[600], step[64];
    int i = 0;
    for (const char *c = pre2_phase; *c && i < 60; c++) if ((*c >= 'A' && *c <= 'Z') || (*c >= 'a' && *c <= 'z') || (*c >= '0' && *c <= '9')) step[i++] = *c;
    step[i] = 0;
    snprintf(path, sizeof path, "%s/%04d_%s.bmp", host_shots_dir, n++, step);
    if (getenv("PRE2_VGADUMP")) {
        const VgaState *v = &pre2_vga;
        fprintf(stderr, "%s: crtc", path);
        for (int r = 0; r < 25; r++) fprintf(stderr, " %02X", v->crtc[r]);
        fprintf(stderr, "\n   seq %02X %02X %02X %02X %02X gc5 %02X gc6 %02X attrIdx %02X attr", v->seq[0], v->seq[1], v->seq[2], v->seq[3], v->seq[4], v->gc[5], v->gc[6], v->attr_index);
        for (int r = 0; r < 21; r++) fprintf(stderr, " %02X", v->attr[r]);
        fprintf(stderr, "\n   dac0-15");
        for (int r = 0; r < 16; r++) fprintf(stderr, " %02X%02X%02X", v->dac[r][0], v->dac[r][1], v->dac[r][2]);
        int nz = 0; for (int q = 0; q < 4; q++) for (int o = 0; o < 0x2000; o++) nz += v->plane[q][o] != 0;
        int nz2 = 0; for (int q = 0; q < 4; q++) for (int o = 0x2000; o < 0x4000; o++) nz2 += v->plane[q][o] != 0;
        fprintf(stderr, "\n   nonzero bytes 0000-1FFF %d, 2000-3FFF %d\n", nz, nz2);
    }
    FILE *f = fopen(path, "wb");
    if (!f) return;
    uint8_t h[54] = { 'B', 'M' };
    uint32_t size = 54 + (uint32_t)(tex_w * tex_h * 4);
    memcpy(h + 2, &size, 4); h[10] = 54; h[14] = 40;
    int32_t w = tex_w, hh = -tex_h; memcpy(h + 18, &w, 4); memcpy(h + 22, &hh, 4);
    h[26] = 1; h[28] = 32;
    fwrite(h, 1, 54, f); fwrite(pixels, 4, (size_t)(tex_w * tex_h), f); fclose(f);
}

/* ---- the service point ---------------------------------------------------------------------- */
static void service(int waiting)
{
    uint64_t now = SDL_GetTicksNS();
#ifdef _WIN32
    last_service_ns = now;
#endif
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) hw.quit = 1;
        if ((e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) || e.type == SDL_EVENT_KEY_UP) {
            int ext; uint8_t code = pc_scancode(e.key.scancode, &ext);
            if (code) run_int09((uint8_t)(code | (e.type == SDL_EVENT_KEY_UP ? 0x80 : 0)));
        }
    }
    if (host_input_script) host_input_script(host_stats.frames_waited, run_int09);
    if (!hw.next_tick_ns) hw.next_tick_ns = now;
    int budget = 64;
    while (now >= hw.next_tick_ns && budget--) {          /* due timer interrupts */
        run_int08(); host_stats.ticks++;
        hw.next_tick_ns += tick_period_ns();
    }
    if (now >= hw.next_tick_ns) hw.next_tick_ns = now;     /* far behind: do not catch up */
    /* the block's IRQ fires when playback has drained the queue to one block (the DMA block
     * "completed"); it starts the next block, so IRQs follow the 8403 Hz / 168 byte rate */
    if (audio && hw.block_pending && SDL_GetAudioStreamQueued(audio) < 168 * 2) { hw.block_pending = 0; run_sb_irq(); }
    bus_pay();
    now = SDL_GetTicksNS();
    crt_advance(now);
    if (crt.frame_ready) present();
    if (host_shots_dir && now >= next_shot_ns) { shot(); next_shot_ns = now + 500000000ull; }
    if (waiting) { host_stats.frames_waited++; SDL_DelayNS(500000); }
    if (hw.quit || (host_stop_after_ns && now >= host_stop_after_ns)) {
        host_shutdown();
        exit(hw.quit ? 0 : 3);
    }
}

/* Load time (pre2_host_io): a reference 386-class machine with a hard disk and no EMS/XMS,
 * where every resource is read from disk and decoded on each use. A file open (directory
 * search, seek) costs 30 ms; each byte read costs 10 us, folding the transfer (~500 KB/s)
 * and the decoding of the compressed stream by the CPU (the larger share) into one rate
 * of ~100 KB/s of compressed input. MAP.SQZ (36 KiB) then takes ~0.4 s, the MODE screen's
 * CODE.TRK and MOTIF.SQZ ~0.2 s together. --fast-load sets both costs to zero. */
int host_fast_load = 0;
static void host_io(uint32_t opens, uint32_t bytes)
{
    if (host_fast_load) return;
    uint64_t end = SDL_GetTicksNS() + opens * 30000000ull + bytes * 10000ull;
    while (SDL_GetTicksNS() < end) service(1);
}

static uint8_t status_3da(void)
{
    static uint64_t last_service;
    bus_pay();
    uint64_t now = SDL_GetTicksNS();
    if (now - last_service > 1000000ull) { last_service = now; service(0); }
    crt_advance(now);
    CrtGeom g = crt_geom();
    double pos = (now - crt.frame_ns) / g.line_ns;          /* lines since line 0 */
    int line = (int)pos;
    int retrace = line >= g.vrs && line < g.vre;
    int inactive = line >= g.vde || (pos - line) >= g.hde_frac;
    return (uint8_t)((retrace ? 0x08 : 0x00) | (inactive ? 0x01 : 0x00));
}

void (*host_input_script)(uint64_t frame, void (*key)(uint8_t)) = 0;
uint64_t host_stop_after_ns = 0;

/* Watchdog (smoke test): when the wall-clock limit passes and the game has not reached a
 * service point since, sample the main thread's instruction pointer a few times, print it
 * (map with addr2line) and stop: a busy-wait that never polls is a host integration bug. */
#ifdef _WIN32
static HANDLE main_thread;
static int SDLCALL watchdog(void *u)
{
    (void)u;
    for (;;) {
        SDL_Delay(500);
        uint64_t now = SDL_GetTicksNS();
        if (host_stop_after_ns && now > host_stop_after_ns + 2000000000ull && now - last_service_ns > 1000000000ull) {
            for (int k = 0; k < 5; k++) {
                SuspendThread(main_thread);
                CONTEXT c; memset(&c, 0, sizeof c); c.ContextFlags = CONTEXT_CONTROL;
                if (GetThreadContext(main_thread, &c)) fprintf(stderr, "watchdog: main thread RIP %016llx\n", (unsigned long long)c.Rip);
                ResumeThread(main_thread);
                SDL_Delay(50);
            }
            fprintf(stderr, "watchdog: no service point for %.1f s; frames %llu ticks %llu\n",
                    (now - last_service_ns) / 1e9, (unsigned long long)host_stats.frames_waited, (unsigned long long)host_stats.ticks);
            fprintf(stderr, "watchdog: module base %016llx\n", (unsigned long long)(uintptr_t)GetModuleHandle(NULL));
            fflush(stderr);
            ExitProcess(4);
        }
    }
    return 0;
}
#endif

int host_init(void)
{
#ifdef _WIN32
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &main_thread, 0, FALSE, DUPLICATE_SAME_ACCESS);
    SDL_CreateThread(watchdog, "watchdog", NULL);
#endif
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) { fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()); return 1; }
    win = SDL_CreateWindow("Prehistorik 2", 960, 600, SDL_WINDOW_RESIZABLE);
    ren = win ? SDL_CreateRenderer(win, NULL) : NULL;
    tex = ren ? SDL_CreateTexture(ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, 320, 200) : NULL;
    if (!tex) { fprintf(stderr, "SDL video: %s\n", SDL_GetError()); return 1; }
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    SDL_SetRenderLogicalPresentation(ren, 320, 200, SDL_LOGICAL_PRESENTATION_LETTERBOX);   /* 4:3 at 320x200 or 640x480 */
    SDL_AudioSpec src = { SDL_AUDIO_U8, 1, 8403 };
    audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &src, NULL, NULL);
    if (audio) SDL_ResumeAudioStreamDevice(audio);
    ports_install(&device);
    vga_status_hook = status_3da;
    vga_access_hook = crt_hook;
    pre2_host_poll = service;
    pre2_host_io = host_io;
    return 0;
}

void host_shutdown(void)
{
    host_stats.end_ns = SDL_GetTicksNS();
    if (host_report) host_report();
    if (audio) SDL_DestroyAudioStream(audio);
    SDL_Quit();
}

void (*host_report)(void) = 0;
