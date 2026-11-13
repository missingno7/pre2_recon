#include "native/sqz.h"
#include "native/assetfs.h"
#include "native/machine.h"
#include "c/pre2.h"
#include "c/sqz.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv) {
    const char *dir = argc > 2 ? argv[2] : "assets"; DIR *d = opendir(dir); struct dirent *e;
    unsigned files = 0, failed = 0; clock_t native_time = 0, game_time = 0;
    if (!d) return 1;
    while ((e = readdir(d))) {
        size_t len = strlen(e->d_name); uint8_t *in = NULL, *out = NULL; size_t n = 0, out_n = 0; uint16_t seg = 0;
        if (len < 4 || (strcmp(e->d_name + len - 4, ".SQZ") && strcmp(e->d_name + len - 4, ".TRK"))) continue;
        rt_boot(dir, 1995, 6, 1); asset_set_dir(dir);
        if (asset_read(e->d_name, &in, &n)) { printf("%s read failed\n", e->d_name); failed++; continue; }
        const char *codec = in[0] == 0xb4 && in[1] == 0x4c ? "LZSS" : in[1] == 0x10 ? "LZW" : "Huffman";
        /* The loader reserves the header size rounded to paragraphs, plus one.
         * Fill that region so extra bytes written after the native result are visible. */
        unsigned hi = (unsigned)in[0] | (unsigned)in[1] << 8;
        unsigned lo = (unsigned)in[2] | (unsigned)in[3] << 8;
        if (!strcmp(codec, "LZSS")) { hi = in[14] >> 2; lo = (unsigned)in[15] | (unsigned)in[16] << 8; }
        size_t allocation = (size_t)(uint16_t)((((hi << 16) | lo) >> 4) + 1) * 16;
        memset(pre2_mem + lin(0x3000, 0), 0xa5, allocation);
        clock_t t = clock(); int a = sqz_decode(in, n, &out, &out_n); native_time += clock() - t;
        SET_DW(DS_NextFreeSegment, 0x3000); SET_CW(CS_ConventionalTopSeg, 0xa000);
        for (size_t i = 0; i <= len; i++) wr8(DATA_SEG, (uint16_t)(0xf000 + i), (uint8_t)e->d_name[i]);
        t = clock(); int b = LoadSqzFile(0xf000, &seg); game_time += clock() - t;
        int mismatch = a || b || out_n > allocation || memcmp(out, pre2_mem + lin(seg, 0), out_n);
        if (!mismatch) for (size_t i = out_n; i < allocation; i++) {
            if (pre2_mem[lin(seg, 0) + i] != 0xa5) { printf("  suffix offset %zu value %02x allocation %zu\n", i, pre2_mem[lin(seg, 0) + i], allocation); mismatch = 1; break; }
        }
        printf("%s %s %zu -> %zu %s\n", e->d_name, codec, n, out_n, mismatch ? "FAIL" : "OK");
        if (mismatch) { if (!a && !b) { size_t i = 0; while (i < out_n && out[i] == pre2_mem[lin(seg, 0) + i]) i++; printf("  first difference %zu\n", i); } failed++; }
        files++; free(in); free(out);
    }
    closedir(d);
    printf("%u files, %u failures; native %.3f s, game %.3f s\n", files, failed, (double)native_time / CLOCKS_PER_SEC, (double)game_time / CLOCKS_PER_SEC);
    return failed || !files;
}

