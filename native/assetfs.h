/* Whole-file reads from the asset directory, resolved as c/dos_files.c resolves DOS names. */
#ifndef NATIVE_ASSETFS_H
#define NATIVE_ASSETFS_H
#include <stddef.h>
#include <stdint.h>
void asset_set_dir(const char *dir);
int asset_read(const char *name, uint8_t **data, size_t *n);
#endif
