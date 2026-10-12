/* VGA hardware model at the level the game's code observes it.
 *
 * Planar video memory (4 x 64 KiB) behind the A000h window with the Sequencer map mask
 * and memory mode (chain-4), the Graphics Controller (set/reset, colour compare, data
 * rotate/function, read map, write/read mode, bit mask) and its latches, CRTC and
 * Attribute registers (stored; start address and panning feed presentation), the DAC
 * (index/sub-index cycling) and the input status register 3DAh (retrace phase supplied
 * by the timing model). One instance is shared by the oracle harness (Unicorn MMIO and
 * port hooks) and the C translation, so both sides run on identical hardware semantics.
 */
#ifndef VGA_H
#define VGA_H
#include <stdint.h>

typedef struct VgaState {
    uint8_t plane[4][0x10000];
    uint8_t latch[4];
    uint8_t seq_index, seq[8];
    uint8_t gc_index, gc[16];
    uint8_t crtc_index, crtc[32];
    uint8_t attr_index, attr[32], attr_flipflop;
    uint8_t misc_output;
    uint8_t dac[256][3];
    uint8_t dac_write_index, dac_read_index, dac_component, dac_read_component;
    uint8_t pel_mask;
    uint32_t status_reads;      /* 3DAh reads since reset (retrace model input) */
    uint8_t retrace_period;     /* status toggles vertical retrace every N reads (0 = never) */
} VgaState;

extern VgaState pre2_vga;

void    vga_reset(void);
uint8_t vga_read(uint16_t offset);              /* CPU read at A000:offset */
void    vga_write(uint16_t offset, uint8_t v);  /* CPU write at A000:offset */
uint8_t vga_in(uint16_t port);
void    vga_out(uint16_t port, uint8_t v);
int     vga_port(uint16_t port);                /* nonzero for ports the model owns */

/* MOVSB and XCHG with A000h:off through the model: a read loads the latches, then the write
 * (XCHG returns the old byte). */
static inline void vga_movsb(uint16_t si, uint16_t di) { vga_write(di, vga_read(si)); }
static inline uint8_t vga_xchg(uint16_t off, uint8_t v)
{
    uint8_t old = vga_read(off);
    vga_write(off, v);
    return old;
}
#endif
