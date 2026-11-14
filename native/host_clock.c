#include "host_clock.h"
void host_clock_init(HostClock *c,uint64_t wall) {c->wall=wall; c->logical=0; c->paused=0;}
uint64_t host_clock_bound(HostClock *c,uint64_t wall)
{
    c->logical=host_clock_target(c,wall);
    c->wall=wall; return c->logical;
}
uint64_t host_clock_target(const HostClock *c,uint64_t wall)
{return c->logical+(!c->paused && wall>c->wall?wall-c->wall:0);}
void host_clock_pause(HostClock *c,int paused,uint64_t wall)
{
    /* Deliberately do not grant a new bound at pause: the worker completes only
     * its already admitted slice. Resume discards all intervening wall time. */
    c->wall=wall; c->paused=!!paused;
}
