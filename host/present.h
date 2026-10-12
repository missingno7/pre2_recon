#ifndef PRESENT_H
#define PRESENT_H
#include <stdint.h>
/* Render the modelled VGA state as the game's 16-colour planar display (see present.c). */
void present_vga(uint32_t *out, int width, int height);
#endif
