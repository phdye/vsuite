#include <stdio.h>
#include <string.h>
#include "vsuite/varchar.h"
#include "vsuite/zvarchar.h"
#include "varchar-logFile.h"

static int failures = 0;
static int verbose = 0;
FILE *logFile;

#define CHECK(name, expr) do { \
    if (!(expr)) { \
        printf("\nFAIL: %s\n", name); \
        failures++; \
    } else if (verbose) { \
        printf("PASS: %s\n", name); \
    } else { \
        fputc('.', stdout); fflush(stdout); \
    } \
} while (0)

/* Verify FIND_FIRST_NUL_BYTE returns the first terminator or NULL. */
static void test_find_first_nul_byte(void) {
    char buf[] = "ab\0cd";
    CHECK("find_nul_found", FIND_FIRST_NUL_BYTE(buf, sizeof buf) == buf + 2);
    char no_nul[4] = {'a','b','c','d'};
    CHECK("find_nul_none", FIND_FIRST_NUL_BYTE(no_nul, 4) == NULL);
}

/* VARCHAR_SETLENZ should place a terminator after the current length. */
static void test_setlenz(void) {
    VARCHAR(v, 4);
    strcpy(v.arr, "ab");
    v.len = 2;
    v.arr[3] = 'x';
    VARCHAR_SETLENZ(v);
    CHECK("setlenz_basic", v.len == 2 && v.arr[2] == '\0' && v.arr[3] == 'x');
}

/* Overflow lengths are truncated by zv_zero_terminate within the macro. */
static void test_setlenz_overflow(void) {
    VARCHAR(v, 4);
    memcpy(v.arr, "abcd", 4);
    v.len = 4;
    VARCHAR_SETLENZ(v);
    CHECK("setlenz_overflow", v.len == 3 && v.arr[3] == '\0');
}

/* VARCHAR_ZSETLEN sets len to the index of the first terminator. */
static void test_zsetlen(void) {
    VARCHAR(v, 5);
    strcpy(v.arr, "abc");
    v.arr[3] = '\0';
    v.len = 0;
    VARCHAR_ZSETLEN(v);
    CHECK("zsetlen", v.len == 3);
}

/* The logging variant of v_copy performs no copy itself. */
static void test_v_copy_noop(void) {
    VARCHAR(src, 4); VARCHAR(dst, 4);
    strcpy(src.arr, "abc");
    src.len = 3;
    dst.len = 0;
    dst.arr[0] = '\0';
    VARCHAR_v_copy(dst, src);
    CHECK("v_copy_noop", dst.len == 0 && dst.arr[0] == '\0');
}

/* Basic formatting should update the destination length. */
static void test_varchar_sprintf(void) {
    VARCHAR(v, 8);
    VARCHAR_sprintf(v, "hi %d", 7);
    CHECK("varchar_sprintf", v.len == 4 && strcmp(v.arr, "hi 7") == 0);
}

int main(int argc, char **argv) {
    for (int i=1;i<argc;i++)
        verbose |= (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose"));

    logFile = fopen("/dev/null", "w");
    if (!logFile) {
        perror("fopen");
        return 1;
    }

    test_find_first_nul_byte();
    test_setlenz();
    test_setlenz_overflow();
    test_zsetlen();
    test_v_copy_noop();
    test_varchar_sprintf();

    fclose(logFile);

    if (failures == 0)
        printf(verbose ? "\nAll tests passed.\n" : "\n");
    else
        printf("\n%d test(s) failed.\n", failures);
    return failures;
}
