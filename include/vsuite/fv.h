#ifndef VSUITE_FV_H
#define VSUITE_FV_H

#include <string.h>
#include <stddef.h>

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

#endif /* VSUITE_FV_H */
