#ifndef PRE2_POOLS_H
#define PRE2_POOLS_H

#include <stdint.h>

/* Logical role is carried with a record. In particular, byte offsets shared by
 * different original arenas do not identify a record's owner. */
typedef enum Pre2PoolKind {
    PRE2_POOL_CHILD,
    PRE2_POOL_BURST,
    PRE2_POOL_PROJECTION,
    PRE2_POOL_DEBRIS,
    PRE2_POOL_POINT,
    PRE2_POOL_TERRAIN
} Pre2PoolKind;

enum {
    PRE2_POOL_ORIGINAL_CHILD = 12,
    PRE2_POOL_ORIGINAL_BURST = 32,
    PRE2_POOL_ORIGINAL_PROJECTION = 20,
    PRE2_POOL_ORIGINAL_DEBRIS = 16,
    PRE2_POOL_ORIGINAL_POINT = 20,
    PRE2_POOL_ORIGINAL_TERRAIN = 7
};

/* DATA offsets for a logical record. Original entries retain their historical
 * addresses and order; wide-only entries occupy the reserved DATA tail. */
unsigned pre2_pool_count(Pre2PoolKind kind);
uint16_t pre2_pool_slot(Pre2PoolKind kind, unsigned index);
int pre2_pool_role(uint16_t offset, Pre2PoolKind *kind, unsigned *index);
unsigned pre2_pool_original_count(Pre2PoolKind kind);
int pre2_pool_grow(Pre2PoolKind kind);
unsigned pre2_pool_highwater(Pre2PoolKind kind);
void pre2_pool_note_use(Pre2PoolKind kind, unsigned index);
unsigned pre2_render_count(void);
uint16_t pre2_render_slot(unsigned index);
int pre2_render_contains(uint16_t offset);
int pre2_pool_is_wide(void);

/* Call after the original B1BCh heap boundary is initialized, before any DOS
 * allocations. It keeps all extension records below the heap in DS. */
void pre2_pools_reserve(void);
/* Call at the original render-slot reset boundary to initialise extension
 * records for the new run/level. */
void pre2_pools_reset(void);

#endif
