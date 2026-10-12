#ifndef PLATFORM_HOST_H
#define PLATFORM_HOST_H
#include <stdint.h>

typedef struct HostStats {
    uint64_t ticks;          /* timer interrupts delivered */
    uint64_t frames_waited;  /* calls of the frame tick wait (platform_tick_wait) */
    uint64_t presents;       /* frames shown */
    uint64_t sb_irqs;        /* Sound Blaster block interrupts delivered */
    uint64_t sb_bytes;       /* PCM bytes queued to the audio device */
    uint64_t end_ns;
} HostStats;
extern HostStats host_stats;

int  host_init(void);
void host_shutdown(void);

/* Smoke-test hooks: a scripted key source (called at every service with the number of
 * frame-wait polls so far; `key` delivers one scan code through the game's INT 09h), a
 * wall-clock limit after which the host stops (exit status 3), and a report at shutdown. */
extern void (*host_input_script)(uint64_t polls, void (*key)(uint8_t));
extern uint64_t host_stop_after_ns;
extern void (*host_report)(void);
#endif
