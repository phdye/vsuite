#include <stdio.h>
#include <string.h>
#include "vsuite/v.h"
#include "vsuite/vf.h"

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
    VARCHAR(dst, 6);
    vf_copy(dst, "abc");
    CHECK("vf_copy len", dst.len == 3 && memcmp(dst.arr, "abc", 3) == 0);
}

static void test_copy_overflow(void) {
    VARCHAR(dst, 3);
    vf_copy(dst, "abcd");
    CHECK("vf_copy overflow", dst.len == 0);
}

static void test_copy_empty(void) {
    VARCHAR(dst, 4);
    vf_copy(dst, "");
    CHECK("vf_copy empty", dst.len == 0);
}

int main(int argc, char **argv) {
    for (int i=1;i<argc;i++) if (!strcmp(argv[i], "-v")) verbose = 1;
    test_copy();
    test_copy_overflow();
    test_copy_empty();
    if (failures == 0) printf(verbose ? "\nAll tests passed.\n" : "\n");
    else printf("\n%d test(s) failed.\n", failures);
    return failures;
}
