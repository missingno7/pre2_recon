/* DOS-model platform services used by system.c (oracle: the INT 10h, INT 67h, XMS and
 * Sound Blaster interactions of src/MODULE1.ASM). Each function replaces one interaction
 * with the machine or the BIOS/DOS environment; the native host supplies its own. The
 * EMS, XMS and Sound Blaster services are not modelled: they return PLAT_UNSUPPORTED so
 * the caller stops at that boundary with a distinct status. */
#ifndef PLATFORM_DOS_H
#define PLATFORM_DOS_H
#include <stdint.h>

enum PlatformStatus {
    PLAT_OK = 0,
    PLAT_UNSUPPORTED = 300      /* boundary service not modelled (see system.h) */
};

/* INT 10h AX=1012h: sets `count` DAC registers from `first`, each three bytes (R, G, B)
 * read from seg:off onward (16-bit offset wrap). Written as the BIOS does it: port 3C8h
 * for the index, then 3C9h for each component, so the VGA model holds the palette. */
int platform_dac_set_block(uint16_t seg, uint16_t off, uint16_t first, uint16_t count);

/* INT 67h AH=44h/4401h: map the pages of EMS handle `handle` for a resource whose
 * ResourceAddressTable entry is `source`. Not modelled: PLAT_UNSUPPORTED. */
int platform_ems_map_pages(uint16_t handle, uint16_t source);

/* XMS AH=0Bh move through XmsDriverEntry for the resource at ResourceAddressTable entry
 * `source`. Not modelled: PLAT_UNSUPPORTED. */
int platform_xms_move(uint16_t handle, uint16_t source);

/* Sound Blaster block start (StartSbBlock: DSP and 8237 DMA programming). Not modelled:
 * PLAT_UNSUPPORTED. */
int platform_sb_start_block(void);
#endif
