/* Credits and intro screens (oracle: src/MODULE2.ASM cs:23D4h..2639h and the picture loader
 * at cs:2642h..26A7h; see credits.c). Routines keep their ASM names. The CF results are the
 * carry of the ASM register contract: 1 = the gate failed, nothing drawn. */
#ifndef CREDITS_H
#define CREDITS_H
#include <stdint.h>

/* INT 1Ah AH=04h into ClockYear. *carry = 1 on a clock error or when the year is 2067 or
 * later, else the carry of the final compare (1 when the year was stored). Returns AX. */
uint16_t ReadClockYear(int *carry);

/* "still working in <year>" credits when ClockYear >= 1996 (7CCh). */
void ShowLateYearCredits(void);

/* Credits screen when the key gate passes (W held; no other key, Ctrl or Alt). Returns CF. */
int TryShowCredits(void);

/* Unused in the binary: credits screen with a timed wait (word_27F0 up to 168h). */
void ShowCreditsTimed(void);

/* Clears the text page and draws the credit lines. Mode 0Dh must already be set. */
void ShowCreditsScreen(void);

/* Intro picture when the key gate passes (E held; no other key, Ctrl or Alt). Returns CF. */
int TryShowIntroKeyed(void);

/* Intro picture when ClockYear >= 1994 (7CAh). Returns CF (1 = nothing done). */
int ShowIntroPicture(void);

/* Shared body: loads the three picture files, sets mode 12h, the palette and the DAC, copies
 * the planes into VRAM, waits for a button press and restores mode 0Dh. Returns CF = 0. */
int ShowIntroBody(void);
#endif
