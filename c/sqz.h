/* SQZ resource loader (oracle: sub_107B). */
#ifndef SQZ_H
#define SQZ_H
#include <stdint.h>

enum {
    SQZ_FATAL_OPEN = 101,      /* loc_1012: "Disk missing" prompt and retry */
    SQZ_FATAL_READ,            /* loc_0FFF: "read error" */
    SQZ_FATAL_MEMORY,          /* loc_06D5: "Not enough memory" */
    SQZ_FATAL_SQUEEZED,        /* loc_0FD9: "squeezed file error" */
    SQZ_LZSS_INSTEAD,          /* internal: LZW header check failed, use LZSS */
    SQZ_UNSUPPORTED            /* a path not translated yet (EMS/XMS sources) */
};

/* Load and decode the SQZ file named at DATA:name; *out_seg = its segment. */
int LoadSqzFile(uint16_t name, uint16_t *out_seg);
#endif
