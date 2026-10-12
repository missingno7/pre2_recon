/* DOS file services (see dos_files.c). Each returns 0 on success, nonzero for the DOS
 * carry-set case with the DOS error code in *ax. */
#ifndef DOS_FILES_H
#define DOS_FILES_H
#include <stdint.h>

int dos_open(uint16_t seg, uint16_t off, uint16_t *ax);              /* AH=3Dh, read only */
int dos_close(uint16_t handle, uint16_t *ax);                        /* AH=3Eh */
int dos_read(uint16_t handle, uint16_t seg, uint16_t off, uint16_t count, uint16_t *ax); /* AH=3Fh */
int dos_seek(uint16_t handle, uint8_t mode, uint32_t offset, uint32_t *pos);            /* AH=42h */
#endif
