#ifndef VSUITE_FIXED_H
#define VSUITE_FIXED_H

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

/* Copy from Fixed VARCHAR to Fixed C String */
#define fv_copy(cdst, vsrc)                                                   \
    do {                                                                      \
        if ((vsrc).len < sizeof(cdst)) {                                      \
            memcpy(cdst, V_BUF(vsrc), (vsrc).len);                            \
            cdst[(vsrc).len] = '\0';                                          \
        } else if (sizeof(cdst) > 0) {                                        \
            cdst[0] = '\0';                                                   \
        }                                                                     \
    } while (0)

#endif /* VSUITE_FIXED_H */
