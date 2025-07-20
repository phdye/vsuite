#ifndef VSUITE_VF_H
#define VSUITE_VF_H

#include <string.h>
#include <stddef.h>

/* Copy from Fixed C String to Fixed VARCHAR */
#define vf_copy(vdst, csrc)                                                   \
    do {                                                                      \
        size_t __n = strlen(csrc);                                            \
        if (__n <= V_SIZE(vdst)) {                                            \
            memcpy(V_BUF(vdst), csrc, __n);                                   \
            (vdst).len = __n;                                                 \
        } else {                                                              \
            (vdst).len = 0;                                                   \
        }                                                                     \
    } while (0)

#endif /* VSUITE_VF_H */
