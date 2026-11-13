/* Pure SQZ decoder: the three codecs of the game's LoadSqzFile (c/sqz.c) as a function of the
 * file bytes, for native asset loading without the emulated memory. Verified byte for byte
 * against LoadSqzFile on every shipped SQZ/TRK file (tests/native/sqz.c). Returns 0 or nonzero. */
#ifndef NATIVE_SQZ_H
#define NATIVE_SQZ_H
#include <stddef.h>
#include <stdint.h>
int sqz_decode(const uint8_t *in, size_t n, uint8_t **out, size_t *out_n);
#endif
