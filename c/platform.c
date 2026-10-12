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
#include "platform.h"

#define ATTR_PAS 0x20u

void platform_poll(void) { }

void bios_set_mode(uint8_t mode)
{
    wr8(0x0040, 0x0049, mode);                 /* BIOS data area: current video mode */
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
