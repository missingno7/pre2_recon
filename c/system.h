/* System services of the original (oracle: src/MODULE1.ASM, see system.c): resource slot
 * activation and paging (disk path), the sound-effect selector, input polling (keyboard
 * flags, joystick, demo log), the text and palette helpers, the timer-vector swap and the
 * keyboard interrupt handler.
 *
 * Boundaries return a distinct status instead of running on: EMS, XMS and the LZSS
 * decoder from conventional memory (resource locations 1..3), the Sound Blaster block
 * start, and the ESC quit path of KeyboardIsr (SYS_QUIT, nothing of the quit is run). */
#ifndef SYSTEM_H
#define SYSTEM_H
#include <stdint.h>
#include "platform_dos.h"

enum SystemStatus {
    SYS_OK = 0,
    SYS_UNSUPPORTED = PLAT_UNSUPPORTED,   /* a boundary path, not translated (see above) */
    SYS_QUIT = 301                        /* KeyboardIsr: ESC quit path entered, not run */
};

/* Starts sound effect DL (sample mode uses the whole DX). */
void StartSoundEffect(uint16_t dx);

/* Makes resource slot `slot` the active slot (see the ASM header for the steps). */
int ActivateResourceSlot(uint16_t slot);

/* Pages resource `slot` in by its location code (ResourceFlags bits 02h and 04h). */
int PageInResource(uint16_t slot);

/* Returns DS offset of the name of resource `index` in the table at DATA:09E0h. */
uint16_t ResourceNameAt(uint16_t index);

void SetStartupPalette(void);
void SetLevelPalette(void);
void SetDacBlock0To15IfFlag(uint16_t seg, uint16_t dx);

void WaitForButtonPressRelease(void);
void PrintDecimalWord(uint16_t dx);
uint16_t PrintAsciiz(uint16_t seg, uint16_t si);
void DrawFontChar(uint8_t al);

void CalibrateJoystick(void);
/* Returns CF (1 when there is no joystick or the lines do not start low); on success
 * *out_x and *out_y receive the counts (AX and BX of the ASM). */
int ReadJoystickTimes(uint16_t *out_x, uint16_t *out_y);
void PollInputState(void);

void SwapInt08Int09Vectors(void);
int KeyboardIsr(void);
#endif
