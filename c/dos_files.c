/* DOS file services the game uses (INT 21h 3Dh open, 3Eh close, 3Fh read, 42h seek),
 * over host files in a resource directory. Same handle numbering, error codes and
 * request log as tools/emu.py:DosFiles, so differential tests can compare the request
 * sequence. The native host uses the same service, and charges each open and read the
 * time it took on a period machine (pre2_host_io, see platform.h). */
#include "pre2.h"
#include "dos_files.h"
#include "platform.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_FILES 15
static struct { FILE *f; int used; } files[MAX_FILES];   /* host handles, not game state */
static char root[512] = "assets";
static char logbuf[1 << 16];
static size_t loglen;

static void logf_(const char *s)
{
    size_t n = strlen(s);
    if (loglen && loglen + 1 < sizeof logbuf) logbuf[loglen++] = ';';
    if (loglen + n < sizeof logbuf) { memcpy(logbuf + loglen, s, n); loglen += n; }
    logbuf[loglen] = 0;
}

EXPORT void pre2_files_reset(const char *dir)
{
    for (int i = 0; i < MAX_FILES; i++) if (files[i].used) { fclose(files[i].f); files[i].used = 0; }
    if (dir) { strncpy(root, dir, sizeof root - 1); root[sizeof root - 1] = 0; }
    loglen = 0; logbuf[0] = 0;
}

EXPORT const char *pre2_files_log(void) { return logbuf; }

int dos_open(uint16_t seg, uint16_t off, uint16_t *ax)
{
    char name[129], path[700], line[200];
    int i;
    for (i = 0; i < 128; i++) { name[i] = (char)rd8(seg, (uint16_t)(off + i)); if (!name[i]) break; }
    name[i] = 0;
    snprintf(line, sizeof line, "open,%s", name); logf_(line);
    for (i = 0; name[i]; i++) name[i] = (char)toupper((unsigned char)name[i]);
    snprintf(path, sizeof path, "%s/%s", root, name);
    FILE *f = fopen(path, "rb");
    if (!f) { *ax = 2; return 1; }
    int h = 5;                                     /* DOS: the lowest free handle (0..4 are standard) */
    while (h - 5 < MAX_FILES && files[h - 5].used) h++;
    if (h - 5 >= MAX_FILES) { fclose(f); *ax = 4; return 1; }
    files[h - 5].f = f; files[h - 5].used = 1;
    *ax = (uint16_t)h;
    if (pre2_host_io) pre2_host_io(1, 0);
    return 0;
}

static FILE *handle(uint16_t h) { return (h >= 5 && h - 5 < MAX_FILES && files[h - 5].used) ? files[h - 5].f : 0; }

int dos_close(uint16_t h, uint16_t *ax)
{
    FILE *f = handle(h);
    if (!f) { *ax = 6; return 1; }
    char line[40]; snprintf(line, sizeof line, "close,%u", h); logf_(line);
    fclose(f); files[h - 5].used = 0;
    return 0;
}

int dos_read(uint16_t h, uint16_t seg, uint16_t off, uint16_t count, uint16_t *ax)
{
    FILE *f = handle(h);
    if (!f) { *ax = 6; return 1; }
    uint8_t buf[65536];
    size_t n = fread(buf, 1, count, f);
    for (size_t i = 0; i < n; i++) wr8(seg, (uint16_t)(off + i), buf[i]);
    char line[60]; snprintf(line, sizeof line, "read,%u,%u,%u", h, count, (unsigned)n); logf_(line);
    *ax = (uint16_t)n;
    if (pre2_host_io) pre2_host_io(0, (uint32_t)n);
    return 0;
}

int dos_seek(uint16_t h, uint8_t mode, uint32_t offset, uint32_t *pos)
{
    FILE *f = handle(h);
    if (!f) { *pos = 6; return 1; }
    char line[60]; snprintf(line, sizeof line, "seek,%u,%u,%lu", h, mode, (unsigned long)offset); logf_(line);
    long base = 0;
    if (mode == 1) base = ftell(f);
    else if (mode == 2) { fseek(f, 0, SEEK_END); base = ftell(f); }
    long target = (long)((uint32_t)(base + (long)offset));
    fseek(f, target, SEEK_SET);
    *pos = (uint32_t)target;
    return 0;
}
