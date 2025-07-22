#ifndef VARCHAR_LOGFILE_H
#define VARCHAR_LOGFILE_H

#include <stdio.h>

#include <vsuite.h>

extern FILE *rptFile;

/*
 * Apply C-string zero-byte terminator to a VARCHAR if 
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
 
// Macro to find the first null byte in a character array
// up to a specified maximum length.
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

#define VARCHAR_ZSETLEN(v)                     \
    {                                          \
        unsigned siz = sizeof((v).arr);        \
        char *nul = FIND_FIRST_NUL_BYTE((v).arr, siz); \
        if (nul == NULL) {                     \
            fprintf(logFile, "Line %d : VARCHAR_ZSETLEN(%s) : No NUL byte found within %u sizeof(.arr) bytes : value '%s'\n", \
                 __LINE__, #v, siz, (v).arr);  \
        }                                      \
        (v).len = (unsigned short)((unsigned long)nul - (unsigned long)(v).arr); \
        (v).arr[siz-1] = '\0'; \
    }

#define VARCHAR_v_copy(dst,src) \
    { \
        unsigned siz = v_capacity(dst); \
        if (siz < (src).len) { \
            fprintf(logFile,"Line %d : v_copy(%s, %s) overflow : destination capacity %u < %u source length\n\n", \
                __LINE__, #dst, #src, siz, (src).len); \
        } \
    }

#define VARCHAR_sprintf(v, fmt, ...) \
    { \
        int ok = v_sprintf_fcn(V_BUF(v), V_SIZE(v), &(v).len, fmt, ##__VA_ARGS__); \
        int actual = sprintf((v).arr, fmt, ##__VA_ARGS__); \
        if (actual > ok) { \
            fprintf(logFile,"Line %d : sprintf(%s,...) overflow : length %d exceeds allocated size %lu\n\n", \
                __LINE__, #v, actual, sizeof((v).arr)); \
        } \
    }

#endif // VARCHAR_LOGFILE_H
