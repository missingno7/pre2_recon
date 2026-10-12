/* DOS-model platform services for system.c (see platform_dos.h). The EMS, XMS and Sound
 * Blaster services are boundaries that report PLAT_UNSUPPORTED. */
#include "pre2.h"
#include "platform_dos.h"

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
