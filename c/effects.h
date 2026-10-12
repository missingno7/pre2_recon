/* Contact, effect, debris and cell-change routines (oracle: src/MODULE3.ASM cs:8295..94F3,
 * minus the front-end screens of screens.c). Names keep the ASM labels; the register contract
 * of each routine is the comment above it in effects.c. Routines of the other regions are called
 * through their owners' headers (player.h, objects.h, level.h, screens.h). */
#ifndef EFFECTS_H
#define EFFECTS_H
#include <stdint.h>

void ProcessPlayerContacts(void);
void SpawnPlayerBurst(void);
void SpawnDebrisElement(uint16_t ax, uint16_t si);
void UpdateFollowerSlots(void);
void BuildEffectDrawList(void);
int  TriggerCellChangeGroup(uint16_t si);
int  TickCellChangeTimer(uint16_t di, uint16_t si);
int  ApplyCellChangeRecord(uint16_t di);
void SetEffectOriginFromRecord(uint16_t di);
uint16_t RandomDebrisFrame(void);
int  ApplyFollowerHitToObjects(uint16_t si);
void DestroyObjectWithDebris(uint16_t di, uint16_t bx, uint16_t si);
void SpawnEffectBurst(uint16_t ax, uint16_t *dx, uint16_t cx);
int  TestHitboxOverlap(uint16_t si, uint16_t di);
uint16_t sub_93F6(uint16_t si);
uint16_t sub_93B2(uint16_t si, uint16_t *di_out);
uint16_t sub_94DC(uint16_t dx, uint16_t ax);
void SpawnFourDebrisParticles(void);

/* Password code from the BIOS seed and CpuClass (AX in, AX out). */
uint16_t ComputePasswordCode(uint16_t ax_in);

#endif
