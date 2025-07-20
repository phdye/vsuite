#ifndef VSUITE_VD_H
#define VSUITE_VD_H

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <vsuite/v.h>

/* Copy from Fixed VARCHAR to Dynamic C String (preallocated dest) */
#define dv_copy(dstr, dcap, vsrc)                                             \
    do {                                                                      \
        if ((vsrc).len < (dcap)) {                                            \
            memcpy(dstr, V_BUF(vsrc), (vsrc).len);                            \
            dstr[(vsrc).len] = '\0';                                          \
        } else if ((dcap) > 0) {                                              \
            dstr[0] = '\0';                                                   \
        }                                                                     \
    } while (0)


#define vd_dup(v) vd_dup_fcn(V_BUF(v), (v).len)

/* Duplicate Fixed VARCHAR into newly allocated Dynamic C String */
static inline char *vd_dup_fcn(const char *src_buf, unsigned short src_len)
{
    char *d = malloc(src_len + 1);
    if (!d) return NULL;
    memcpy(d, src_buf, src_len);
    d[src_len] = '\0';
    return d;
}


#endif /* VSUITE_VD_H */
