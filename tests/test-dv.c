#include <stdio.h>
#include <string.h>
#include "vsuite/v.h"
#include "vsuite/dv.h"
#include <stdlib.h>

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
    char dst[6];
    VARCHAR(src, 6);
    strcpy(src.arr, "abc"); src.len = 3;
    dv_copy(dst, sizeof(dst), src);
    CHECK("dv_copy len", strcmp(dst, "abc") == 0);
}

static void test_copy_overflow(void) {
    char dst[4];
    VARCHAR(src, 6);
    strcpy(src.arr, "abcd"); src.len = 4;
    dv_copy(dst, sizeof(dst), src);
    CHECK("dv_copy overflow", dst[0] == '\0');
}

static void test_copy_empty(void) {
    char dst[4];
    VARCHAR(src, 4);
    src.len = 0; src.arr[0] = '\0';
    dv_copy(dst, sizeof(dst), src);
    CHECK("dv_copy empty", dst[0] == '\0');
}

static void test_copy_zero_cap(void) {
    char dst[1];
    VARCHAR(src, 2);
    strcpy(src.arr, "a"); src.len = 1;
    dst[0] = 'x';
    dv_copy(dst, 0, src);
    CHECK("dv_copy zero cap", dst[0] == 'x');
}

static void test_dup(void) {
    VARCHAR(src, 6);
    strcpy(src.arr, "abc"); src.len = 3;
    char *d = dv_dup(src);
    CHECK("dv_dup", d && strcmp(d, "abc") == 0);
    free(d);
}

int main(int argc, char **argv) {
    for (int i=1;i<argc;i++) if (!strcmp(argv[i], "-v")) verbose = 1;
    test_copy();
    test_copy_overflow();
    test_copy_empty();
    test_copy_zero_cap();
    test_dup();
    if (failures == 0) printf(verbose ? "\nAll tests passed.\n" : "\n");
    else printf("\n%d test(s) failed.\n", failures);
    return failures;
}
