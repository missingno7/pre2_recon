/* Player update, player physics and tile collision, projectiles and debris (oracle:
 * src/MODULE3.ASM cs:5850..6821). See player.c for the register contracts. */
#ifndef PLAYER_H
#define PLAYER_H
#include <stdint.h>

enum PlayerStatus {
    PLAYER_OK = 0,
    PLAYER_UNSUPPORTED = 401,  /* a handler word read from a DATA table is not a translated routine
                                  (data words past the tables, or a target outside this region) */
    PLAYER_CREDITS = 402       /* TryShowCredits gate passed: credits screen path not translated */
};

/* Per-frame player update (MainFrameLoop calls it after UpdateTerrainEntities). `cx_in` is the
 * CX the frame leaves on entry: the original reads it on one attack path (see player.c). */
int UpdatePlayer(uint16_t cx_in);

/* Player collision after the integrate: foot cell, ground and ceiling tile contact, and the
 * side-wall column scan. UpdatePlayerTileContact takes the foot cell in BX. */
int  UpdatePlayerCollision(void);
int  UpdatePlayerTileContact(uint16_t bx);

/* Projectile, particle and debris slots (called from MainFrameLoop). */
int  UpdatePlayerProjectiles(void);
void UpdateBurstParticles(void);
void TickDebrisSlots(void);
int  FindFreeProjectileSlot(uint16_t *si_out);

/* Tile handlers reached through the DATA tables (DI = map cell, see player.c). */
void LandPlayerOnGround(uint16_t di);
void GroundTileSnapOrFall(uint16_t di);
void CeilingTileHeadBump(uint16_t di);
void SideTileWallImpactHandler(uint16_t di);
int  RedrawTileIfVisible(uint16_t ax, uint16_t di);

void TriggerPlayerRespawnCore(void);
void IncrementPlayerLives(void);
void sub_67D7(void);
#endif
