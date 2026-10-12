/* Platform services for the test model (see platform.h). The model keeps the effects the
 * game can observe and nothing else:
 *   platform_poll        a no-op: the tests end the busy-waits by setting the tick counter
 *                        in their setup, so a wait ends on its first test. The native host
 *                        replaces this with its own pump/sleep.
 *   bios_set_mode        records the current video mode in the BIOS data area (0040:0049h),
 *                        as the BIOS does. VRAM is not cleared (not modelled here).
 *   bios_set_palette     INT 10h AX=1002h: index/data writes to the attribute controller
 *                        (3C0h) for registers 0..0Fh and the overscan register 11h. Video
 *                        output stays enabled (PAS set).
 *   bios_set_dac_block   INT 10h AX=1012h: DAC index 'first', then count*3 data bytes.
 *   dos_print_dollar     INT 21h AH=09h: console output, not modelled (no effect).
 * The writes go through port_out8, so the VGA model sees the same register traffic as the
 * oracle's INT 10h handler (tests/render.py).
 */
#include "pre2.h"
#include "ports.h"
#include "vga.h"
#include "platform.h"

#define ATTR_PAS 0x20u

/* The native host installs pre2_host_poll; the test model leaves it unset. */
void (*pre2_host_poll)(int waiting) = 0;
void (*pre2_host_io)(uint32_t opens, uint32_t bytes) = 0;

void platform_poll(void) { if (pre2_host_poll) pre2_host_poll(0); }

/* Test tick model (off by default): each failed check of the frame wait advances
 * TimerTickCount by one, as tools/difftest.py's tick_model hook does on the oracle side. */
static int tick_model;                              /* test configuration, not game state */
EXPORT void pre2_platform_tick_model(int on) { tick_model = on; }
void platform_tick_wait(void)
{
    if (pre2_host_poll) { pre2_host_poll(1); return; }
    if (tick_model) SET_CW(CS_TimerTickCount, (uint16_t)(CW(CS_TimerTickCount) + 1));
}

uint16_t platform_conventional_kb(void) { return 640; }

/* INT 10h AH=00h: the VGA part (registers, default palette, memory clear) is the model's
 * vga_bios_mode_set; the BIOS data area fields below are the ones a mode set writes.
 * tests/bios.py does the same for the oracle side. */
void bios_set_mode(uint8_t al)
{
    uint8_t mode = al & 0x7F;
    uint16_t cols = 40, page = 0x2000, rows = 24, charh = 8;
    if (mode == 0x03) { cols = 80; page = 0x1000; charh = 16; }
    else if (mode == 0x12) { cols = 80; page = 0xA000; rows = 29; charh = 16; }
    else if (mode == 0x13) { page = 0xFA00; }
    vga_bios_mode_set(al);
    wr8(0x0040, 0x0049, mode);
    wr16(0x0040, 0x004A, cols);
    wr16(0x0040, 0x004C, page);
    wr16(0x0040, 0x004E, 0);
    for (uint16_t i = 0; i < 16; i++) wr8(0x0040, (uint16_t)(0x0050 + i), 0);
    wr8(0x0040, 0x0062, 0);
    wr16(0x0040, 0x0063, 0x03D4);
    wr8(0x0040, 0x0084, (uint8_t)rows);
    wr16(0x0040, 0x0085, charh);
    wr8(0x0040, 0x0087, (uint8_t)((rd8(0x0040, 0x0087) & 0x7F) | (al & 0x80)));
}

void bios_set_palette(uint16_t seg, uint16_t off)
{
    (void)port_in8(0x3DA);                     /* reset the attribute flip-flop */
    for (uint16_t i = 0; i < 0x10; i++) {
        port_out8(0x3C0, (uint8_t)(i | ATTR_PAS));
        port_out8(0x3C0, rd8(seg, (uint16_t)(off + i)));
    }
    port_out8(0x3C0, (uint8_t)(0x11 | ATTR_PAS));      /* overscan */
    port_out8(0x3C0, rd8(seg, (uint16_t)(off + 0x10)));
}

void bios_set_dac_block(uint16_t first, uint16_t count, uint16_t seg, uint16_t off)
{
    port_out8(0x3C8, (uint8_t)first);
    for (uint32_t k = 0; k < (uint32_t)count * 3; k++)
        port_out8(0x3C9, rd8(seg, (uint16_t)(off + k)));
}

void dos_print_dollar(uint16_t seg, uint16_t off)
{
    (void)seg; (void)off;
}
