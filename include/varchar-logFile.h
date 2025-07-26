#ifndef VARCHAR_LOGFILE_H
#define VARCHAR_LOGFILE_H

#include <stdio.h>

#include <vsuite.h>

/* Required log destination used by the debugging wrappers.
 * It might be stderr or an actual file.  It should be unbuffered to ensure
 * that messages are written immediately to preclude loss of information
 * in case of a crash.
 */
extern FILE *logFile;

#define VARCHAR_v_valid(v)                                        \
    ({                                                            \
        size_t capacity = V_SIZE(v);                              \
        if ((v).len > capacity) {                                 \
            fprintf(logFile,                                      \
                    "Line %d : v_valid(%s) overflow : .len %u > %lu c-string capacity\n\n", \
                    __LINE__, #v, (v).len, capacity);             \
        }                                                         \
        v_valid(v);                                               \
    })

#define VARCHAR_zv_valid(v)                                       \
    ({                                                            \
        size_t capacity = ZV_CAPACITY(v);                         \
        if ((v).len > capacity) {                                 \
            fprintf(logFile,                                      \
                    "Line %d : zv_valid(%s) overflow : .len %u > %lu c-string capacity\n\n", \
                    __LINE__, #v, (v).len, capacity);             \
        } else {                                                  \
            if (V_BUF(v)[(v).len] != '\0') {                      \
                fprintf(logFile,                                  \
                    "Line %d : zv_valid(%s) : c-string not zero-byte terminated\n\n", \
                    __LINE__, #v);                                \
            }                                                     \
        }                                                         \
        zv_valid(v);                                              \
    })

/*
 * VARCHAR_SETLENZ() - Safely NUL terminate a VARCHAR.
 * @v: VARCHAR variable to modify.
 *
 * Ensures ``v`` contains a valid terminator while printing a diagnostic
 * message to ``logFile`` if the current length is out of bounds or the
 * buffer lacks space for the terminator.
 */
#define VARCHAR_SETLENZ(v) \
    do { \
        if ( !v_valid(v)) { \
            fprintf(logFile,"Line %d : VARCHAR_SETLENZ:  %s : length %u exceeds allocated size %lu\n\n", \
                __LINE__, #v, (v).len, sizeof((v).arr)); \
        } else { \
            if ( !v_has_unused_capacity(v, 1)) { \
                fprintf(logFile,"Line %d : VARCHAR_SETLENZ:  %s does not have an unused byte for the string terminator\n\n", \
                    __LINE__, #v); \
            } \
        } \
        zv_zero_terminate(v); \
    } while (0)

/*
 * FIND_FIRST_NUL_BYTE() - Locate the first NUL in ``arr``.
 * @arr:     Character array to scan.
 * @max_len: Maximum number of bytes to examine.
 *
 * Returns a pointer to the first ``'\0'`` within the given range or ``NULL``
 * when none is present.
 */
#define FIND_FIRST_NUL_BYTE(arr,max_len)       \
    ({                                         \
        char * found = NULL;                   \
        for (size_t i = 0; i < max_len; ++i) { \
            if (arr[i] == '\0') {              \
                found = arr + i;               \
                break;                         \
            }                                  \
        }                                      \
        found;                                 \
    })

/*
 * VARCHAR_ZSETLEN() - Determine the length of a NUL terminated VARCHAR.
 * @v: VARCHAR variable whose ``len`` should reflect the first terminator.
 *
 * Scans the buffer up to its declared size looking for ``'\0'``.  ``v.len`` is
 * updated to the index of that terminator.  When none is found a diagnostic is
 * written and ``v.len`` becomes an undefined large value, but the last array
 * element is forced to ``'\0'`` so subsequent operations remain bounded.
 */
#define VARCHAR_ZSETLEN(v)                     \
    {                                          \
        unsigned siz = sizeof((v).arr);        \
        char *nul = FIND_FIRST_NUL_BYTE((v).arr, siz); \
        if (nul == NULL) {                     \
            fprintf(logFile, "Line %d : VARCHAR_ZSETLEN(%s) : No NUL byte found within %u sizeof(.arr) bytes : value '%s'\n",\
                    __LINE__, #v, siz, (v).arr); \
            nul = (v).arr + siz - 1; /* point to the last byte */ \
        }                                       \
        (v).len = (unsigned short)((unsigned long)nul - (unsigned long)(v).arr); \
        (v).arr[(v).len] = '\0';                  \
    }

/*
 * VARCHAR_v_copy() - Log potential overflow of ``v_copy``.
 * @dst: Destination VARCHAR passed to ``v_copy``.
 * @src: Source VARCHAR passed to ``v_copy``.
 *
 * The macro duplicates the capacity check used by ``v_copy`` and prints a
 * message to ``logFile`` when ``src`` would not fit inside ``dst``.
 */
#define VARCHAR_v_copy(dst,src)                                   \
    do {                                                          \
        VARCHAR_v_valid(src);                                     \
        unsigned siz = V_SIZE(dst);                               \
        if (siz < (src).len) {                                    \
            fprintf(logFile,                                      \
                    "Line %d : v_copy(%s, %s) overflow : destination capacity %u < %u source length\n\n", \
                    __LINE__, #dst, #src, siz, (src).len);        \
        }                                                         \
        /* logging only: do not copy */                           \
    } while (0)

/*
 * VARCHAR_zv_copy() - Log potential overflow of ``zv_copy``.
 * @dst: Destination VARCHAR passed to ``zv_copy``.
 * @src: Source VARCHAR passed to ``zv_copy``.
 *
 * The macro duplicates the capacity check used by ``zv_copy`` and prints a
 * message to ``logFile`` when ``src`` would not fit inside ``dst``.
 */
#define VARCHAR_zv_copy(dst,src)                                  \
    do {                                                          \
        VARCHAR_zv_valid(src);                                    \
        unsigned cap = ZV_CAPACITY(dst);                          \
        if ( cap < (src).len) {                                   \
            fprintf(logFile,                                      \
                    "Line %d : zv_copy(%s, %s) overflow : destination c-string capacity %u < %u source length\n\n", \
                    __LINE__, #dst, #src, cap, (src).len);        \
        }                                                         \
        zv_copy((dst), (src));                                    \
    } while (0)

/*
 * VARCHAR_zvp_copy() - Log potential overflow of ``zvp_copy``.
 * @dst: Destination VARCHAR passed to ``zvp_copy``.
 * @src: Source char* passed to ``zvp_copy``.
 *
 * The macro duplicates the capacity check used by ``zvp_copy`` and prints a
 * message to ``logFile`` when ``src`` would not fit inside ``dst``.
 */
#define VARCHAR_zvp_copy(dst,src)                                 \
    do {                                                          \
        unsigned len = strlen(src);                               \
        unsigned cap = ZV_CAPACITY(dst);                          \
        if ( cap < len) {                                         \
            fprintf(logFile,                                      \
                    "Line %d : zvp_copy(%s, %s) overflow : destination c-string capacity %u < %u source length\n\n", \
                    __LINE__, #dst, #src, cap, len);              \
        }                                                         \
        zvp_copy((dst), (src));                                   \
    } while (0)

/*
 * VARCHAR_sprintf() - Format into a VARCHAR while validating size.
 * @v:   Destination VARCHAR.
 * @fmt: ``printf`` style format string.
 * @...: Arguments consumed according to ``fmt``.
 *
 * The macro calls ``v_sprintf_fcn`` to perform a bounds checked formatting
 * operation and then mirrors the same call with ``sprintf`` for debugging
 * purposes.  If ``sprintf`` reports writing more characters than allowed the
 * mismatch is logged to ``logFile``.
 */
#define VARCHAR_sprintf(v, fmt, ...)                               \
    ({                                                             \
        unsigned capacity = ZV_CAPACITY(v);                        \
        int n = v_sprintf_fcn(V_BUF(v), capacity, &(v).len, fmt, ##__VA_ARGS__); \
        if (varchar_overflow > 0) {                                \
            fprintf(logFile,                                       \
                    "Line %d : sprintf(%s,...) overflow : length %lu exceeds allocated size %lu\n\n", \
                    __LINE__, #v, V_SIZE(v)+varchar_overflow, V_SIZE(v)); \
        } \
        n; \
    })

#endif // VARCHAR_LOGFILE_H
