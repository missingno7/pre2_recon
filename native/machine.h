/* The logical machine: deterministic time, interrupt delivery and host-controlled stepping
 * for the translated game (docs/native.md, "Timing boundary").
 *
 * The game (c/game.c ProgramEntry) is the original program's control flow: nested screen
 * loops, busy-waits and frame waits that never return to a caller. It runs on its own
 * coroutine (a fiber) and yields only at the original's service points:
 *   platform_tick_wait   one failed check of the frame tick wait (WaitSoundTicks)
 *   platform_poll        one turn of any other busy-wait
 *   3DAh reads           retrace / display-enable polling (vga_status_hook)
 *   DOS loads            period load time (pre2_host_io)
 * Everything the original's interrupts do runs there, in logical-time order:
 *   timer      PIT channel 0 at 1193182 / divisor Hz (the divisor the game programs)
 *   keyboard   scan codes queued by the host, one INT 09h each
 *   audio      the 168-byte block events of the Sound Blaster model (docs/timing.md)
 *   CRT        vertical retrace / display start edges from the CRTC timing registers
 * Logical time advances only by what the original machine spent: waits (jump to the next
 * event), a busy-wait turn (RT_POLL_TURN_NS), video bus time (0.5 us per memory access, 1 us
 * per port access) and load time (30 ms per open, 10 us per byte). It never depends on
 * wall time or on the audio device, so a run is a function of its input schedule.
 *
 * The host drives it with rt_run_until(T): the game runs until its logical time would pass
 * T (or it exits), then control returns to the host. The host maps wall time to T, shows the
 * frames the machine published, and submits the PCM it produced.
 */
#ifndef PRE2_MACHINE_H
#define PRE2_MACHINE_H
#include <stdint.h>
#include "../c/audio_events.h"

#define RT_NS_PER_SEC 1000000000ull
#define RT_PIT_HZ 1193182ull
#define RT_POLL_TURN_NS 10000ull       /* one busy-wait turn on the reference 386 */

typedef enum RtStatus {
    RT_STEP_DONE = 0,     /* logical time reached the target; the game is suspended */
    RT_EXITED,            /* ProgramEntry returned (rt_exit_code) */
    RT_QUIT,              /* the ESC quit path of KeyboardIsr or rt_request_quit */
} RtStatus;

typedef struct RtStats {
    uint64_t ticks;            /* timer interrupts delivered */
    uint64_t tick_waits;       /* failed frame-wait checks (platform_tick_wait) */
    uint64_t polls;            /* busy-wait turns (platform_poll) */
    uint64_t audio_blocks;     /* audio block events (SB block IRQs) */
    uint64_t pcm_bytes;        /* PCM bytes produced */
    uint64_t display_frames;   /* CRT frames started (70.1 Hz in every mode the game sets) */
    uint64_t bus_ns;           /* logical time charged as video bus time */
    uint64_t load_ns;          /* logical time charged as load time */
} RtStats;

typedef struct RtConfig {
    int fast_load;             /* loads take no logical time */
    int fast_vga;              /* video accesses take no logical time */
    int audio_native;          /* command-driven engine; zero selects the reference SB path */
} RtConfig;

/* Pre-run choice, kept outside RtConfig so existing positional initializers
 * remain source compatible under -Wmissing-field-initializers. Call before
 * rt_init; 0/320 original, 336..640 in multiples of 16 selects the variant. */
int rt_set_wide_sim_width(unsigned width);

/* The machine the game starts on: our ASM build's program image relocated at LOAD_SEG (never
 * PRE2.EXE), the VGA after power-on, the asset directory as drive C:, the RTC date, and the
 * configuration the original probes for: a Sound Blaster at 220h IRQ 7, a 386-class CPU,
 * 640 KiB conventional memory, no EMS/XMS, BIOS ROM fold zero (native/boot.c). */
void rt_boot(const char *assets, int year, int month, int day);
/* Creates the game coroutine (the memory image and platform configuration must be ready). */
int  rt_init(const RtConfig *cfg);
/* Runs the game until logical time `target_ns` (or exit/quit). */
RtStatus rt_run_until(uint64_t target_ns);
uint64_t rt_now(void);
int  rt_exit_code(void);
void rt_request_quit(void);
const RtStats *rt_stats(void);

/* Input: one PC scan code (set 1, make/break; E0h prefixes as separate codes), delivered as
 * an INT 09h at the next service point, in queue order. */
void rt_key(uint8_t code);              /* at the host's present (the last target) */
void rt_key_at(uint8_t code, uint64_t t_ns);   /* at a logical time (scripts, replays) */

/* Observers (host side; optional). Called on the game coroutine, in logical-time order. */
/* A CRT frame starts being displayed at `t_ns` with the start address latched at the
 * vertical retrace before it (the reference scan-out uses it; see host/present.c). */
extern void (*rt_on_display_frame)(uint64_t t_ns, uint16_t start_address);
/* `n` bytes of unsigned 8-bit mono PCM at `rate` Hz start playing at `t_ns`. */
extern void (*rt_on_pcm)(uint64_t t_ns, const uint8_t *pcm, uint32_t n, uint32_t rate);
/* Every service point (scripted input, scenario tracing). */
extern void (*rt_on_service)(void);
/* Every semantic command, stamped at its origin in either audio mode. */
extern void (*rt_on_audio_command)(uint64_t t_ns, const struct AudioCmd *cmd);
#endif
