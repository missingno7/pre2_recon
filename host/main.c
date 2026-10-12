/* Native SDL3 host: runs the translated game (c/game.c ProgramEntry) on the original memory
 * model with host platform services (host/platform_host.c).
 *
 *   pre2 [--assets DIR] [--smoke SECONDS] [--shots DIR] [--fast-load] [--fast-vga]
 *
 * The address space starts as the program image of our own ASM build (embedded by
 * tools/hostbuild.py as pre2_image[], relocated to LOAD_SEG), never PRE2.EXE. Machine
 * configuration the original probes for: a Sound Blaster at 220h, IRQ 7 (the probes are a
 * platform service), a 386-class CPU, 640 KiB conventional memory, no EMS/XMS, the RTC date
 * from the host clock, drive C:. The BIOS ROM area F000:FFF0 reads as zero bytes (the level
 * codes hash it, so codes match those of a machine whose ROM fold is zero).
 *
 * --smoke SECONDS: the bounded integration test (docs/smoke-test.md): scripted key presses
 * get through the start-up screens into the first level, play a few seconds, report tick
 * and frame cadence, and stop.
 */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "../c/pre2.h"
#include "../c/vga.h"
#include "../c/game.h"
#include "../c/startup.h"
#include "platform_host.h"
#include "present.h"

extern const uint8_t pre2_image[];
extern const uint32_t pre2_image_size;
extern const uint32_t pre2_relocations[];
extern const uint32_t pre2_relocation_count;
void pre2_files_reset(const char *dir);
void pre2_platform_rtc(uint16_t ax, uint16_t cx, uint16_t dx, int carry);
void pre2_platform_cpu(int cls);
void pre2_platform_sound(int sb, uint16_t base, int irq, int adlib);
void pre2_platform_misc(uint16_t ss, uint8_t ems_ah, uint8_t drive, uint8_t xms_al);

static void load_image(void)
{
    memset(pre2_mem, 0, MEM_SIZE);
    memcpy(pre2_mem + lin(LOAD_SEG, 0), pre2_image, pre2_image_size);
    for (uint32_t i = 0; i < pre2_relocation_count; i++) {
        uint32_t p = lin(LOAD_SEG, 0) + pre2_relocations[i];
        uint16_t w = (uint16_t)(pre2_mem[p] | pre2_mem[p + 1] << 8);
        w = (uint16_t)(w + LOAD_SEG);
        pre2_mem[p] = (uint8_t)w; pre2_mem[p + 1] = (uint8_t)(w >> 8);
    }
    pre2_mem[lin(LOAD_SEG - 0x10, 0x80)] = 0;            /* PSP command tail: no switches */
}

static uint8_t bcd(int v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }

/* ---- the bounded smoke test ------------------------------------------------------------- */
static uint64_t smoke_start_ns, gameplay_ns, gameplay_ticks;
static uint32_t gameplay_frames0;
static uint64_t gameplay_sb_irqs, gameplay_sb_bytes;
static uint32_t gameplay_mem0, gameplay_io0;
static void smoke_keys(uint64_t polls, void (*key)(uint8_t))
{
    /* Until the first gameplay frame: RETURN tapped (0.1 s, a quick human press) every 1.2 s (start-up screens,
     * title, front end, map: each waits for a press and a release it polls). Then: hold RIGHT. */
    static uint64_t last; static int held, down;
    uint64_t now = SDL_GetTicksNS();
    (void)polls;
    if (!gameplay_ns && pre2_frames_run > 0) {
        gameplay_ns = now; gameplay_ticks = host_stats.ticks; gameplay_frames0 = pre2_frames_run;
        gameplay_sb_irqs = host_stats.sb_irqs; gameplay_sb_bytes = host_stats.sb_bytes;
        gameplay_mem0 = vga_mem_accesses; gameplay_io0 = vga_io_accesses;
        if (down) { key(0x9C); down = 0; }
    }
    if (gameplay_ns) { if (!held) { key(0x4D); held = 1; } return; }
    if (down && now - last >= 100000000ull) { key(0x9C); down = 0; }
    /* hands off while the map scrolls in, so the whole scroll-in can be observed */
    if (!strcmp(pre2_phase, "ShowWorldMapScrollIn()")) return;
    if (!down && now - last >= 1200000000ull) { key(0x1C); down = 1; last = now; }
}

static void smoke_report(void)
{
    double secs = (host_stats.end_ns - smoke_start_ns) / 1e9;
    printf("smoke: ran %.2f s; timer interrupts %llu (%.2f Hz); frames presented %llu\n", secs,
           (unsigned long long)host_stats.ticks, host_stats.ticks / secs, (unsigned long long)host_stats.presents);
    if (gameplay_ns) {
        double g = (host_stats.end_ns - gameplay_ns) / 1e9;
        uint32_t f = pre2_frames_run - gameplay_frames0;
        uint64_t t = host_stats.ticks - gameplay_ticks;
        printf("smoke: gameplay reached after %.2f s; in gameplay %.2f s: %llu ticks (%.2f Hz), %u frames "
               "(%.2f frames/s, %.2f ticks/frame); PlayerX %04X, CurrentLevel %u\n",
               (gameplay_ns - smoke_start_ns) / 1e9, g, (unsigned long long)t, t / g, f, f / g,
               f ? t / (double)f : 0.0, DW(DS_PlayerX), DB(DS_CurrentLevel));
        printf("smoke: in gameplay: Sound Blaster block IRQs %.2f/s, PCM %.0f bytes/s\n",
               (host_stats.sb_irqs - gameplay_sb_irqs) / g, (host_stats.sb_bytes - gameplay_sb_bytes) / g);
        if (f) printf("smoke: in gameplay: VGA accesses per frame: %u memory, %u port (bus time %.1f ms of the 41.2 ms frame)\n",
                      (vga_mem_accesses - gameplay_mem0) / f, (vga_io_accesses - gameplay_io0) / f,
                      ((vga_mem_accesses - gameplay_mem0) * 0.5e-3 + (vga_io_accesses - gameplay_io0) * 1e-3) / f);
    } else printf("smoke: gameplay NOT reached\n");
    printf("smoke: Sound Blaster block IRQs %llu (%.2f/s), PCM bytes %llu (%.0f B/s)\n", (unsigned long long)host_stats.sb_irqs,
           host_stats.sb_irqs / secs, (unsigned long long)host_stats.sb_bytes, host_stats.sb_bytes / secs);
    printf("smoke: last top-level step: %s; main-loop frames %u\n", pre2_phase, pre2_frames_run);
    static uint32_t px[PRESENT_MAX_W * PRESENT_MAX_H];
    int pw, ph;
    present_vga(px, &pw, &ph);
    FILE *f = fopen("smoke_last.bmp", "wb");
    if (f) {
        uint8_t h[54] = { 'B', 'M' };
        uint32_t size = 54 + (uint32_t)(pw * ph * 4);
        memcpy(h + 2, &size, 4); h[10] = 54; h[14] = 40;
        int32_t w = pw, hh = -ph; memcpy(h + 18, &w, 4); memcpy(h + 22, &hh, 4);
        h[26] = 1; h[28] = 32;
        fwrite(h, 1, 54, f); fwrite(px, 4, (size_t)(pw * ph), f); fclose(f);
        printf("smoke: last frame written to smoke_last.bmp\n");
    }
}

int main(int argc, char **argv)
{
    const char *assets = "assets";
    double smoke = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--assets") && i + 1 < argc) assets = argv[++i];
        else if (!strcmp(argv[i], "--smoke") && i + 1 < argc) smoke = atof(argv[++i]);
        else if (!strcmp(argv[i], "--shots") && i + 1 < argc) host_shots_dir = argv[++i];
        else if (!strcmp(argv[i], "--fast-load")) host_fast_load = 1;
        else if (!strcmp(argv[i], "--fast-vga")) host_fast_vga = 1;
    }
    load_image();
    vga_reset();
    pre2_files_reset(assets);
    time_t t = time(NULL); struct tm *lt = localtime(&t);
    int year = lt->tm_year + 1900;
    pre2_platform_rtc(0x0400, (uint16_t)(bcd(year / 100) << 8 | bcd(year % 100)),
                      (uint16_t)(bcd(lt->tm_mon + 1) << 8 | bcd(lt->tm_mday)), 0);
    pre2_platform_cpu(3);
    pre2_platform_sound(1, 0x220, 7, 0);
    pre2_platform_misc(LOAD_SEG, 0x80, 2, 0x00);
    if (host_init()) return 1;
    if (smoke > 0) {
        smoke_start_ns = SDL_GetTicksNS();
        host_stop_after_ns = smoke_start_ns + (uint64_t)(smoke * 1e9);
        host_input_script = smoke_keys;
        host_report = smoke_report;
    }
    int r = ProgramEntry();
    fprintf(stderr, "ProgramEntry returned status %d\n", r);
    host_shutdown();
    return r ? 2 : 0;
}
