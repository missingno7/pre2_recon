#ifndef PRESENT_H
#define PRESENT_H
#include <stdint.h>
/* Render the modelled VGA state as the game's 16-colour planar display (see present.c). */
#define PRESENT_MAX_W 640
#define PRESENT_MAX_H 480
/* *width x *height (320x200, 640x350/480...) are set from the register state. */
void present_vga(uint32_t *out, int *width, int *height);
/* The same with the start address the CRTC latched for the frame (instead of CRTC 0Ch/0Dh). */
void present_vga_start(uint32_t *out, int *width, int *height, uint16_t start);
#endif
