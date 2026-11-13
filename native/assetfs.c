#include "assetfs.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static char root[512] = "assets";
void asset_set_dir(const char *dir) {
    if (dir) { strncpy(root, dir, sizeof root - 1); root[sizeof root - 1] = 0; }
}
int asset_read(const char *name, uint8_t **data, size_t *n) {
    char path[700], upper[129]; size_t i; long len; FILE *f; uint8_t *p;
    if (!name || !data || !n) return 1;
    *data = NULL; *n = 0;
    for (i = 0; i < 128 && name[i]; i++) upper[i] = (char)toupper((unsigned char)name[i]);
    upper[i] = 0;
    if (snprintf(path, sizeof path, "%s/%s", root, upper) >= (int)sizeof path) return 1;
    f = fopen(path, "rb"); if (!f) return 1;
    if (fseek(f, 0, SEEK_END) || (len = ftell(f)) < 0 || fseek(f, 0, SEEK_SET)) { fclose(f); return 1; }
    p = malloc(len ? (size_t)len : 1); if (!p) { fclose(f); return 1; }
    size_t got = fread(p, 1, (size_t)len, f); int close_error = fclose(f);
    if (got != (size_t)len || close_error) { free(p); return 1; }
    *data = p; *n = (size_t)len; return 0;
}
