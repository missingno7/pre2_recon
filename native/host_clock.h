#ifndef PRE2_HOST_CLOCK_H
#define PRE2_HOST_CLOCK_H
#include <stdint.h>
/* Access under the host bridge mutex. Pause freezes the last permitted bound,
 * including when a worker is still completing that bound. */
typedef struct HostClock { uint64_t wall, logical; int paused; } HostClock;
void host_clock_init(HostClock *c,uint64_t wall);
uint64_t host_clock_bound(HostClock *c,uint64_t wall);
/* Read-only current wall target for drift reporting, without admitting a step. */
uint64_t host_clock_target(const HostClock *c,uint64_t wall);
void host_clock_pause(HostClock *c,int paused,uint64_t wall);
#endif
