/* The native compositor (docs/native.md, "Presentation").
 *
 * Consumes the scene events the game emits (c/scene*.h) and draws each operation natively, from
 * decoded assets, into its own video store: indexed pixels in the address layout the game's
 * drawing assumes (pages, tile ring, backdrop and the other off-screen surfaces are regions of
 * it, so the original's page persistence, partial updates and address arithmetic carry over
 * exactly). No VGA registers, latches or write modes are involved: each handler implements the
 * visual effect of one original drawing routine.
 *
 * At each CRT display start the visible image is scanned out of the store (native/scanout.h)
 * into an immutable Pre2Frame (native/frame.h).
 */
#ifndef COMPOSITOR_H
#define COMPOSITOR_H
#include <stdint.h>
#include "../c/scene.h"
#include "frame.h"

/* The video store. Planar modes (0Dh, 12h): the 4-bit colour of dot d (0..7 from the left) of
 * byte address a is comp_planar[a * 8 + d]. Mode 13h (chain 4): the byte at plane q, offset o is
 * comp_chain4[q][o] (CPU address a -> q = a & 3, o = a >> 2). A mode set clears both. */
extern uint8_t comp_planar[0x10000 * 8];
extern uint8_t comp_chain4[4][0x10000];
static inline uint8_t *comp_px(uint16_t addr, int dot) { return &comp_planar[(uint32_t)addr * 8u + (uint32_t)dot]; }

typedef void (*SceneHandler)(const SceneEvent *e);
/* Registers the handler of one kind (the gameplay and screens families register theirs). */
void compositor_handle(uint16_t kind, SceneHandler h);
/* Installs the scene sink and every family's handlers. */
void compositor_init(void);
/* Events that arrived without a handler (diagnostics: a family not implemented yet). */
uint32_t compositor_unhandled(uint16_t *first_kind);

/* Publishes the frame displayed from logical time t with the latched start address. */
void compositor_publish(uint64_t t_ns, uint16_t start, Pre2Frame *out);

/* Families (native/draw_game.c, native/draw_screens.c). */
void draw_game_register(void);
void draw_screens_register(void);
/* Optional hooks a family installs: the semantic gameplay view and the transition state of a
 * frame being published. */
extern void (*compositor_game_view)(FrameGameView *v, uint16_t start);
extern void (*compositor_transition)(FrameTransition *t, uint16_t start);

/* Parity diagnostics (reference runs only, with video memory modelled): compares the whole
 * store with the VGA model's planes in the current mode. Returns the number of differing
 * pixels (planar: dots; chain 4: bytes) and the first differing address. */
uint32_t compositor_diff_vga(uint32_t *first_addr);
#endif
