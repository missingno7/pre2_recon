/* VGA hardware model (see vga.h). Register semantics follow the IBM VGA reference. */
#include "pre2.h"
#include "vga.h"
#include <string.h>

VgaState pre2_vga;
uint8_t (*vga_status_hook)(void) = 0;
void (*vga_access_hook)(void) = 0;
uint32_t vga_mem_accesses, vga_io_accesses;

EXPORT VgaState *pre2_vga_state(void) { return &pre2_vga; }
EXPORT uint32_t pre2_vga_state_size(void) { return sizeof(VgaState); }

EXPORT void vga_reset(void)
{
    memset(&pre2_vga, 0, sizeof pre2_vga);
    pre2_vga.seq[2] = 0x0F;          /* all planes enabled */
    pre2_vga.seq[4] = 0x06;          /* odd/even off, extended memory */
    pre2_vga.gc[8] = 0xFF;           /* bit mask */
    pre2_vga.gc[7] = 0x0F;
    pre2_vga.pel_mask = 0xFF;
}

static int chain4(void) { return pre2_vga.seq[4] & 0x08; }

EXPORT uint8_t vga_read(uint16_t offset)
{
    VgaState *v = &pre2_vga;
    vga_mem_accesses++;
    if (chain4()) {
        uint16_t a = offset >> 2;
        for (int p = 0; p < 4; p++) v->latch[p] = v->plane[p][a];
        return v->plane[offset & 3][a];
    }
    for (int p = 0; p < 4; p++) v->latch[p] = v->plane[p][offset];
    if (v->gc[5] & 0x08) {           /* read mode 1: colour compare */
        uint8_t r = 0xFF;
        for (int p = 0; p < 4; p++) {
            if (!(v->gc[7] & (1 << p))) continue;
            uint8_t want = (v->gc[2] & (1 << p)) ? 0xFF : 0x00;
            r &= (uint8_t)~(v->latch[p] ^ want);
        }
        return r;
    }
    return v->latch[v->gc[4] & 3];
}

static uint8_t alu(uint8_t value, uint8_t latch)
{
    switch ((pre2_vga.gc[3] >> 3) & 3) {
    case 1: return value & latch;
    case 2: return value | latch;
    case 3: return value ^ latch;
    default: return value;
    }
}

EXPORT void vga_write(uint16_t offset, uint8_t value)
{
    VgaState *v = &pre2_vga;
    vga_mem_accesses++;
    if (chain4()) {
        int p = offset & 3;
        if (v->seq[2] & (1 << p)) v->plane[p][offset >> 2] = value;
        return;
    }
    uint8_t rot = v->gc[3] & 7;
    uint8_t rotated = (uint8_t)((value >> rot) | (value << ((8 - rot) & 7)));
    uint8_t mask = v->gc[8];
    int mode = v->gc[5] & 3;
    for (int p = 0; p < 4; p++) {
        if (!(v->seq[2] & (1 << p))) continue;
        uint8_t d, m = mask;
        switch (mode) {
        case 0:
            d = (v->gc[1] & (1 << p)) ? ((v->gc[0] & (1 << p)) ? 0xFF : 0x00) : rotated;
            d = alu(d, v->latch[p]);
            break;
        case 1:
            v->plane[p][offset] = v->latch[p];
            continue;
        case 2:
            d = alu((value & (1 << p)) ? 0xFF : 0x00, v->latch[p]);
            break;
        default:                     /* 3 */
            m = mask & rotated;
            d = alu((v->gc[0] & (1 << p)) ? 0xFF : 0x00, v->latch[p]);
            break;
        }
        v->plane[p][offset] = (uint8_t)((d & m) | (v->latch[p] & ~m));
    }
}

EXPORT int vga_port(uint16_t port) { return port >= 0x3C0 && port <= 0x3DA; }

EXPORT uint8_t vga_in(uint16_t port)
{
    VgaState *v = &pre2_vga;
    vga_io_accesses++;
    if (vga_access_hook) vga_access_hook();
    switch (port) {
    case 0x3C1: return v->attr[v->attr_index & 0x1F];
    case 0x3C4: return v->seq_index;
    case 0x3C5: return v->seq[v->seq_index & 7];
    case 0x3C6: return v->pel_mask;
    case 0x3C7: return 3;
    case 0x3C8: return v->dac_write_index;
    case 0x3C9: {
        uint8_t r = v->dac[v->dac_read_index][v->dac_read_component];
        if (++v->dac_read_component == 3) { v->dac_read_component = 0; v->dac_read_index++; }
        return r;
    }
    case 0x3CC: return v->misc_output;
    case 0x3CE: return v->gc_index;
    case 0x3CF: return v->gc[v->gc_index & 15];
    case 0x3D4: return v->crtc_index;
    case 0x3D5: return v->crtc[v->crtc_index & 31];
    case 0x3DA: {
        v->attr_flipflop = 0;
        if (vga_status_hook) return vga_status_hook();     /* native host: real-time retrace */
        uint32_t n = v->status_reads++;
        uint8_t retrace = v->retrace_period && ((n / v->retrace_period) & 1) ? 0x08 : 0x00;
        return (uint8_t)(retrace | (retrace ? 0x01 : (n & 1)));
    }
    default: return 0xFF;
    }
}

EXPORT void vga_out(uint16_t port, uint8_t value)
{
    VgaState *v = &pre2_vga;
    vga_io_accesses++;
    if (vga_access_hook) vga_access_hook();
    switch (port) {
    case 0x3C0:
        if (!v->attr_flipflop) v->attr_index = value; else v->attr[v->attr_index & 0x1F] = value;
        v->attr_flipflop ^= 1;
        break;
    case 0x3C2: v->misc_output = value; break;
    case 0x3C4: v->seq_index = value; break;
    case 0x3C5: v->seq[v->seq_index & 7] = value; break;
    case 0x3C6: v->pel_mask = value; break;
    case 0x3C7: v->dac_read_index = value; v->dac_read_component = 0; break;
    case 0x3C8: v->dac_write_index = value; v->dac_component = 0; break;
    case 0x3C9:
        v->dac[v->dac_write_index][v->dac_component] = value & 0x3F;
        if (++v->dac_component == 3) { v->dac_component = 0; v->dac_write_index++; }
        break;
    case 0x3CE: v->gc_index = value; break;
    case 0x3CF: v->gc[v->gc_index & 15] = value; break;
    case 0x3D4: v->crtc_index = value; break;
    case 0x3D5: v->crtc[v->crtc_index & 31] = value; break;
    default: break;
    }
}

/* Retrace model for bounded tests: 3DAh reports vertical retrace for `period` reads,
 * then display for `period` reads, alternately (0 = never in retrace). */
EXPORT void vga_set_retrace(uint8_t period) { pre2_vga.retrace_period = period; }
