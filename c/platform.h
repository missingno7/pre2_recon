/* Platform services used by the video routines (see render.c). Each entry replaces one
 * interaction with the machine or the DOS/BIOS environment; the native host supplies its
 * own versions, the tests use the recording versions in platform.c, and the oracle side
 * services the same requests through INT 10h / INT 21h handlers in tests/render.py.
 *
 *   platform_poll()          one turn of a busy-wait (the host may pump events here).
 *   bios_set_mode(mode)      INT 10h AH=00h: set video mode (AL = mode).
 *   bios_set_palette(s, o)   INT 10h AX=1002h: attribute palette + overscan from ES:DX
 *                            (17 bytes at seg:off).
 *   bios_set_dac_block(f,n,s,o) INT 10h AX=1012h: n DAC registers from f, 3 bytes each,
 *                            from seg:off.
 *   dos_print_dollar(s, o)   INT 21h AH=09h: print the '$'-terminated string at DS:DX.
 */
#ifndef PLATFORM_H
#define PLATFORM_H
#include <stdint.h>

void platform_poll(void);
void bios_set_mode(uint8_t mode);
void bios_set_palette(uint16_t seg, uint16_t off);
void bios_set_dac_block(uint16_t first, uint16_t count, uint16_t seg, uint16_t off);
void dos_print_dollar(uint16_t seg, uint16_t off);

#endif
