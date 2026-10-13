/* The original address space (see pre2.h) and its accessors for test harnesses. */
#include "pre2.h"

uint8_t pre2_mem[MEM_SIZE];
EXPORT uint16_t pre2_es = 0;

EXPORT uint8_t *pre2_memory(void) { return pre2_mem; }
EXPORT uint32_t pre2_memory_size(void) { return MEM_SIZE; }
