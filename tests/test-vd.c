#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vsuite/v.h"
#include "vsuite/vd.h"

static int failures = 0;
static int verbose = 0;

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

static void test_copy(void) {
    VARCHAR(src, 6);
    char dst[6];
    strcpy(src.arr, "abc"); src.len = 3;
    vd_copy(dst, sizeof(dst), src);
    CHECK("vd_copy len", strcmp(dst, "abc") == 0);
}

static void test_copy_overflow(void) {
    VARCHAR(src, 6);
    char dst[4];
    strcpy(src.arr, "abcd"); src.len = 4;
    vd_copy(dst, sizeof(dst), src);
    CHECK("vd_copy overflow", dst[0] == '\0');
}

static void test_copy_zero_cap(void) {
    VARCHAR(src, 6);
    char dst[1];
    strcpy(src.arr, "a"); src.len = 1;
    dst[0] = 'x';
    vd_copy(dst, 0, src);
    CHECK("vd_copy zero cap", dst[0] == 'x'); /* unchanged */
}

static void test_dup(void) {
    VARCHAR(src, 6);
    strcpy(src.arr, "abc"); src.len = 3;
    char *d = vd_dup(src);
    CHECK("vd_dup", d && strcmp(d, "abc") == 0);
    free(d);
}

int main(int argc, char **argv) {
    for (int i=1;i<argc;i++) if (!strcmp(argv[i], "-v")) verbose = 1;
    test_copy();
    test_copy_overflow();
    test_copy_zero_cap();
    test_dup();
    if (failures == 0) printf(verbose ? "\nAll tests passed.\n" : "\n");
    else printf("\n%d test(s) failed.\n", failures);
    return failures;
}
