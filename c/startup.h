/* Start-up services of the original (oracle: src/MODULE1.ASM, called from ProgramEntry):
 * ProbeCpuClass (cs:1C0Ch), InitSoundDevices (cs:0802h..0849h) with the Sound Blaster and
 * AdLib set-up that follows detection, InitResourceMemory (cs:08F0h..0A2Bh) and LoadRawFile
 * (cs:0F7Fh). See startup.c for the boundaries.
 *
 * Platform services (each replaces one hardware or DOS interaction; the tests use the
 * recording versions in startup.c, the native host its own):
 *   platform_rtc_read_date   INT 1Ah AH=04h: AX, CX (century:year BCD), DX (month:day BCD),
 *                            returns CF.
 *   platform_cpu_class       the CPU class ProbeCpuClass detects: 0, 2 or 3.
 *   platform_sound_detect    everything the Sound Blaster and AdLib probes do on the bus (port
 *                            scan, DSP reset/read, DSP test block with DMA, IRQ counting
 *                            handlers, OPL timer probe). Returns what they found.
 *   platform_stack_segment   the SS of the original (its value enters the conventional-memory
 *                            check of the Sound Blaster set-up).
 *   platform_ems_int67_40h   INT 67h AH=40h (EMS status), used once a driver name matches.
 *   platform_dos_drive       INT 21h AH=19h (current drive, AL).
 *   platform_xms_installed   INT 2Fh AX=4300h (AL = 80h when an XMS driver is present).
 */
#ifndef STARTUP_H
#define STARTUP_H
#include <stdint.h>

/* Result of the sound probes (DetectSoundBlaster steps 1-3, DetectAdLib). */
typedef struct SoundProbe {
    uint8_t  sb_found;        /* a DSP answered the reset and the DMA test block raised an IRQ */
    uint16_t sb_base;         /* SbBasePort (the DSP base, e.g. 220h) */
    uint8_t  sb_irq;          /* SbIrqLine: IRQ whose counting handler ran once */
    uint8_t  irq_counts[8];   /* IrqProbeCounts for IRQ 0..7 (DATA 0E67h..0E6Eh) */
    uint8_t  adlib_found;     /* OPL2 timer probe passed (probed only without a Sound Blaster) */
} SoundProbe;

void     platform_rtc_read_date(uint16_t *ax, uint16_t *cx, uint16_t *dx, int *carry);
int      platform_cpu_class(void);
void     platform_sound_detect(SoundProbe *out);
uint16_t platform_stack_segment(void);
uint8_t  platform_ems_int67_40h(void);
uint8_t  platform_dos_drive(void);
uint8_t  platform_xms_installed(void);

/* DspWriteWait (cs:1C97h): AL to the DSP write port, busy-waiting while bit 7 is set. */
void DspWriteWait(uint8_t v);

void ProbeCpuClass(void);
int  InitSoundDevices(void);
int  InitResourceMemory(void);
int  LoadRawFile(uint16_t name, uint16_t *seg);

/* Returned by the start-up services at a boundary they do not translate (EMS/XMS
 * resource placement, the DSP failure paths that print a message, DOS open/read errors
 * that print a message and wait for a key). The caller stops with this status. */
enum StartupStatus {
    STARTUP_OK = 0,
    STARTUP_UNSUPPORTED = 300
};
#endif
