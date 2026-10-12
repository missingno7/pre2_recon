/* Per-frame object update (oracle: src/MODULE3.ASM UpdateObjects cs:6822 and the routines in
 * cs:6822..8294: child and entity handlers, boss, level-5 and level-9 routines, culling and
 * the pool allocators). Only the per-frame entry is exported; the routines are reached through
 * UpdateObjects and the adapters in objects.c.
 *
 * UpdateObjects returns OBJ_OK or the boundary that stopped it. */
#ifndef OBJECTS_H
#define OBJECTS_H
#include <stdint.h>

enum ObjectStatus {
    OBJ_OK = 0,
    OBJ_DISPATCH_UNSUPPORTED = 401,  /* a handler table word is not a translated routine */
    OBJ_DIVIDE_ERROR = 402,          /* the original's DIV would raise INT 0 (divide overflow) */
    OBJ_RUNAWAY = 403                /* the original loops forever here (stride 0, lookup miss) */
};

int UpdateObjects(void);

#endif
