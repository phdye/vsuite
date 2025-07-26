#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "vsuite/varchar.h"
#include "vsuite/string.h"

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

/* Test capacity helpers for fixed C strings */
static void test_capacity(void) {
    char buf[5] = "ab";
    CHECK("S_SIZE", S_SIZE(buf) == 5);
    CHECK("s_has_capacity", s_has_capacity(buf, 4));
    CHECK("s_unused_capacity", s_unused_capacity(buf) == 2);
    CHECK("s_has_unused", s_has_unused_capacity(buf, 2));
}

/* s_init and s_clear should blank the buffer */
static void test_init_clear(void) {
    char buf[4];
    strcpy(buf, "xx");
    s_init(buf);
    CHECK("s_init", buf[0] == '\0');
    strcpy(buf, "yy");
    s_clear(buf);
    CHECK("s_clear", buf[0] == '\0' && buf[1] == '\0' && buf[2] == '\0');
}

/* Validate s_valid on good and bad strings */
static void test_valid(void) {
    char ok[4] = "ab";
    CHECK("s_valid ok", s_valid(ok));
    char bad[3] = {'a','b','c'}; /* no terminator */
    CHECK("s_valid bad", !s_valid(bad));
}

/* Copy operations with and without overflow */
static void test_copy(void) {
    char dst[4];
    int n = s_copy(dst, "ab");
    CHECK("s_copy", n == 2 && strcmp(dst, "ab") == 0);
    n = s_copy(dst, "abcd");
    CHECK("s_copy ov", n == 3 && strcmp(dst, "abc") == 0);
}

/* s_strncpy variants */
static void test_strncpy(void) {
    char dst[5];
    int n = s_strncpy(dst, "abcd", 2);
    CHECK("s_strncpy", n == 2 && strcmp(dst, "ab") == 0);
    n = s_strncpy(dst, "abcdef", 5);
    CHECK("s_strncpy ov", n == 4 && strcmp(dst, "abcd") == 0);
}

/* Concatenation helpers */
static void test_strcat(void) {
    char dst[5] = "ab";
    int n = s_strcat(dst, "cd");
    CHECK("s_strcat", n == 2 && strcmp(dst, "abcd") == 0);
    strcpy(dst, "ab");
    char small[4] = "ab";
    n = s_strcat(small, "cde");
    CHECK("s_strcat ov", n == 1 && strcmp(small, "abc") == 0);
}

static void test_strncat(void) {
    char dst[5] = "ab";
    int n = s_strncat(dst, "cdef", 2);
    CHECK("s_strncat", n == 2 && strcmp(dst, "abcd") == 0);
    char tiny[3] = "ab";
    n = s_strncat(tiny, "cd", 2); /* capacity only one char left */
    CHECK("s_strncat ov", n == 0 && strcmp(tiny, "ab") == 0);
}

/* Trimming helpers */
static void test_trim(void) {
    char buf[8];
    strcpy(buf, "  hi  ");
    s_ltrim(buf);
    CHECK("s_ltrim", strcmp(buf, "hi  ") == 0);
    strcpy(buf, "  hi  ");
    s_rtrim(buf);
    CHECK("s_rtrim", strcmp(buf, "  hi") == 0);
    strcpy(buf, "  hi  ");
    s_trim(buf);
    CHECK("s_trim", strcmp(buf, "hi") == 0);
}

/* Case conversion with misc input */
static void test_case(void) {
    char buf[5];
    strcpy(buf, "a1B");
    s_upper(buf);
    CHECK("s_upper", strcmp(buf, "A1B") == 0);
    s_lower(buf);
    CHECK("s_lower", strcmp(buf, "a1b") == 0);
}

/* Stress test with a large buffer */
static void test_mass_upper(void) {
    enum { N = 4096 };
    static char buf[N];
    memset(buf, 'a', N-1);
    buf[N-1] = '\0';
    s_upper(buf);
    int ok = 1;
    for (size_t i = 0; i < N-1; i++) if (buf[i] != 'A') { ok = 0; break; }
    CHECK("s_mass_upper", ok);
}

int main(int argc, char **argv) {
    for (int i=1;i<argc;i++) if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) verbose = 1;

    test_capacity();
    test_init_clear();
    test_valid();
    test_copy();
    test_strncpy();
    test_strcat();
    test_strncat();
    test_trim();
    test_case();
    test_mass_upper();

    if (failures == 0)
        printf(verbose ? "\nAll tests passed.\n" : "\n");
    else
        printf("\n%d test(s) failed.\n", failures);

    return failures;
}
