#include "pre2.h"
#include "pools.h"
#include "view.h"
#include <string.h>

/* The original arenas are not adjacent by role, so extensions live in a
 * separate DATA tail. Keep offsets below C000h and reserve the paragraphs
 * before the game's variable/asset heap starts. */
#define EXT_CHILD       0xB200u
#define EXT_BURST       0xBA28u
#define EXT_PROJECTION  0xC9E8u
#define EXT_DEBRIS      0xCD6Cu
#define EXT_POINT       0xD54Cu
#define EXT_TERRAIN     0xDAD4u
#define EXT_END         0xDB80u

static const uint8_t original_count[] = { 12, 32, 20, 16, 20, 7 };
static const uint16_t initial_count[] = { 24, 64, 70, 32, 40, 16 };
static const uint16_t maximum_count[] = { 128, 256, 70, 128, 256, 16 };
static const uint16_t original_base[] = { 0x4FD0, 0x50A8, 0x52E8, 0x5450, 0x7DE6, 0x5570 };
static const uint16_t extension_base[] = { EXT_CHILD, EXT_BURST, EXT_PROJECTION, EXT_DEBRIS, EXT_POINT, EXT_TERRAIN };
static const uint8_t record_size[] = { 0x12, 0x12, 0x12, 0x12, 6, 0x12 };
static uint16_t current_count[6];
static uint16_t highwater[6];

int pre2_pool_is_wide(void)
{
    return pre2_view.variant != 0;
}

unsigned pre2_pool_count(Pre2PoolKind kind)
{
    if ((unsigned)kind >= sizeof original_count) return 0;
    if (!pre2_pool_is_wide()) return original_count[kind];
    if (!current_count[kind]) current_count[kind] = initial_count[kind];
    return current_count[kind];
}

unsigned pre2_pool_original_count(Pre2PoolKind kind)
{
    return (unsigned)kind < sizeof original_count ? original_count[kind] : 0;
}

int pre2_pool_grow(Pre2PoolKind kind)
{
    if (!pre2_pool_is_wide() || (unsigned)kind >= sizeof original_count) return 0;
    unsigned n = pre2_pool_count(kind);
    if (n >= maximum_count[kind]) return 0;
    unsigned next = n * 2;
    if (next > maximum_count[kind]) next = maximum_count[kind];
    current_count[kind] = (uint16_t)next;
    for (unsigned i = n; i < next; i++) {
        uint16_t at = (uint16_t)(extension_base[kind] + (i - original_count[kind]) * record_size[kind]);
        SET_DW(at + (kind == PRE2_POOL_POINT ? 0 : 4), 0xFFFF);
    }
    return 1;
}

unsigned pre2_pool_highwater(Pre2PoolKind kind)
{
    return (unsigned)kind < sizeof highwater / sizeof highwater[0] ? highwater[kind] : 0;
}

void pre2_pool_note_use(Pre2PoolKind kind, unsigned index)
{
    if ((unsigned)kind < sizeof highwater / sizeof highwater[0] && index + 1 > highwater[kind])
        highwater[kind] = (uint16_t)(index + 1);
}

unsigned pre2_render_count(void)
{
    if (!pre2_pool_is_wide()) return 116;
    return 29 + pre2_pool_count(PRE2_POOL_CHILD) + pre2_pool_count(PRE2_POOL_BURST) +
        pre2_pool_count(PRE2_POOL_PROJECTION) + pre2_pool_count(PRE2_POOL_DEBRIS) +
        pre2_pool_count(PRE2_POOL_TERRAIN);
}

uint16_t pre2_render_slot(unsigned index)
{
    if (!pre2_pool_is_wide()) return index < 116 ? (uint16_t)(0x4F0A + index * 0x12) : 0xFFFF;
    if (index < 11) return (uint16_t)(0x4F0A + index * 0x12);
    index -= 11;
    const Pre2PoolKind kinds[] = { PRE2_POOL_CHILD, PRE2_POOL_BURST, PRE2_POOL_PROJECTION,
        PRE2_POOL_DEBRIS, PRE2_POOL_TERRAIN };
    for (unsigned k = 0; k < sizeof kinds / sizeof kinds[0]; k++) {
        unsigned n = pre2_pool_count(kinds[k]);
        if (index < n) return pre2_pool_slot(kinds[k], index);
        index -= n;
    }
    if (index < 18) return (uint16_t)(0x4F0A + (98 + index) * 0x12);
    return 0xFFFF;
}

uint16_t pre2_pool_slot(Pre2PoolKind kind, unsigned index)
{
    if ((unsigned)kind >= sizeof original_count || index >= (pre2_pool_is_wide() ? maximum_count[kind] : original_count[kind])) return 0xFFFF;
    if (index < original_count[kind])
        return (uint16_t)(original_base[kind] + index * record_size[kind]);
    return (uint16_t)(extension_base[kind] + (index - original_count[kind]) * record_size[kind]);
}

int pre2_pool_role(uint16_t offset, Pre2PoolKind *kind, unsigned *index)
{
    for (unsigned k=0;k<6;k++) for (unsigned i=0;i<pre2_pool_count((Pre2PoolKind)k);i++)
        if (pre2_pool_slot((Pre2PoolKind)k,i)==offset) {
            if (kind) *kind=(Pre2PoolKind)k;
            if (index) *index=i;
            return 1;
        }
    return 0;
}

int pre2_render_contains(uint16_t offset)
{
    for (unsigned i=0;i<pre2_render_count();i++) if (pre2_render_slot(i)==offset) return 1;
    return 0;
}

void pre2_pools_reserve(void)
{
    if (!pre2_view.configured) return;
    /* NextFreeSegment initially names B1C0h (DATA-relative). Round the gap
     * through the reserved extension arenas before ordinary heap use. */
    uint16_t paras = (uint16_t)((EXT_END - 0xB1BCu + 15u) >> 4);
    SET_DW(DS_NextFreeSegment, (uint16_t)(DW(DS_NextFreeSegment) + paras));
}

static void init_records(Pre2PoolKind kind, uint16_t sentinel_offset)
{
    for (unsigned i = original_count[kind]; i < initial_count[kind]; i++) {
        uint16_t at = pre2_pool_slot(kind, i);
        SET_DW(at + sentinel_offset, 0xFFFF);
    }
}

void pre2_pools_reset(void)
{
    memset(current_count, 0, sizeof current_count);
    memset(highwater, 0, sizeof highwater);
    if (!pre2_pool_is_wide()) return;
    for (unsigned kind = 0; kind < 6; kind++) current_count[kind] = initial_count[kind];
    init_records(PRE2_POOL_CHILD, 4);
    init_records(PRE2_POOL_BURST, 4);
    init_records(PRE2_POOL_PROJECTION, 4);
    init_records(PRE2_POOL_DEBRIS, 4);
    init_records(PRE2_POOL_POINT, 0);
    init_records(PRE2_POOL_TERRAIN, 4);
}
