/* The machine the native game starts on (see machine.h rt_boot). */
#include "machine.h"
#include "../c/pre2.h"
#include "../c/vga.h"
#include "assetfs.h"
#include <string.h>

extern const uint8_t pre2_image[];
extern const uint32_t pre2_image_size;
extern const uint32_t pre2_relocations[];
extern const uint32_t pre2_relocation_count;
void pre2_files_reset(const char *dir);
void pre2_platform_rtc(uint16_t ax, uint16_t cx, uint16_t dx, int carry);
void pre2_platform_cpu(int cls);
void pre2_platform_sound(int sb, uint16_t base, int irq, int adlib);
void pre2_platform_misc(uint16_t ss, uint8_t ems_ah, uint8_t drive, uint8_t xms_al);

static uint8_t bcd(int v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }

void rt_boot(const char *assets, int year, int month, int day)
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
    vga_reset();
    pre2_files_reset(assets);
    asset_set_dir(assets);                              /* native asset loading reads the same files */
    pre2_platform_rtc(0x0400, (uint16_t)(bcd(year / 100) << 8 | bcd(year % 100)),
                      (uint16_t)(bcd(month) << 8 | bcd(day)), 0);
    pre2_platform_cpu(3);
    pre2_platform_sound(1, 0x220, 7, 0);
    pre2_platform_misc(LOAD_SEG, 0x80, 2, 0x00);
}
