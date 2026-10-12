/* DOS-model platform services for system.c (see platform_dos.h). The DAC write goes
 * through the port model, so the VGA palette state is the same on both sides of a test. */
#include "pre2.h"
#include "ports.h"
#include "platform_dos.h"

int platform_dac_set_block(uint16_t seg, uint16_t off, uint16_t first, uint16_t count)
{
    port_out8(0x3C8, (uint8_t)first);
    for (uint32_t i = 0; i < (uint32_t)count * 3; i++)
        port_out8(0x3C9, rd8(seg, (uint16_t)(off + i)));
    return PLAT_OK;
}

int platform_ems_map_pages(uint16_t handle, uint16_t source)
{
    (void)handle; (void)source;
    return PLAT_UNSUPPORTED;
}

int platform_xms_move(uint16_t handle, uint16_t source)
{
    (void)handle; (void)source;
    return PLAT_UNSUPPORTED;
}

int platform_sb_start_block(void)
{
    return PLAT_UNSUPPORTED;
}
