#ifndef VSUITE_DV_H
#define VSUITE_DV_H

#include <string.h>
#include <stddef.h>

/* Copy from Dynamic C String to Fixed VARCHAR */
#define vd_copy(vdst, dsrc)                                                   \
    do {                                                                      \
        size_t __n = strlen(dsrc);                                            \
        if (__n < V_SIZE(vdst)) {                                             \
            memcpy(V_BUF(vdst), dsrc, __n);                                   \
            (vdst).len = __n;                                                 \
        } else {                                                              \
            (vdst).len = 0;                                                   \
        }                                                                     \
    } while (0)

#endif /* VSUITE_DV_H */
