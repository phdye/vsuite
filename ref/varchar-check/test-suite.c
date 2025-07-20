#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "varchar-check.h"

#define DECL_VARCHAR(name, size) struct { unsigned short len; char arr[size]; } name

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

static void expect_abort(void (*fn)(void), const char *name) {
    pid_t pid = fork();
    if (pid == 0) {
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        fn();
        exit(0);
    } else {
        int status; waitpid(pid, &status, 0);
        CHECK(name, WIFSIGNALED(status));
    }
}

static void abort_check_len(void) {
    DECL_VARCHAR(v, 4);
    v.len = 4; // equal to capacity
    VARCHAR_CHECK_LEN(v);
}

static void abort_check_str(void) {
    DECL_VARCHAR(v, 4);
    v.len = 3;
    memcpy(v.arr, "abc", 3); // fill without NUL
    v.arr[3] = 'x';
    VARCHAR_CHECK_STR(v);
}

static void abort_zsetlen_overflow(void) {
    DECL_VARCHAR(v, 3);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
    strcpy(v.arr, "abcd");
#pragma GCC diagnostic pop
    VARCHAR_ZSETLEN(v);
}

static void abort_setlenz_overflow(void) {
    DECL_VARCHAR(v, 3);
    v.len = 3;
    VARCHAR_SETLENZ(v);
}

static void abort_copy_small_dest(void) {
    DECL_VARCHAR(src, 5); DECL_VARCHAR(dst, 3);
    strcpy(src.arr, "abcd"); src.len = 4;
    VARCHAR_COPY(dst, src);
}

static void abort_copy_in_overflow(void) {
    DECL_VARCHAR(v, 3);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
    VARCHAR_COPY_IN(v, "abcd");
#pragma GCC diagnostic pop
}

static void abort_copy_out_small(void) {
    DECL_VARCHAR(v, 4); char out[3];
    strcpy(v.arr, "abc"); v.len = 3;
    VARCHAR_COPY_OUT(out, v);
}

void test_init(void) {
    DECL_VARCHAR(v, 5); memset(v.arr, 'X', sizeof(v.arr)); v.len = 2;
    VARCHAR_INIT(v);
    int allzero = 1; for (size_t i=0;i<sizeof(v.arr);i++) if(v.arr[i]!=0) allzero=0;
    CHECK("VARCHAR_INIT clears", allzero && v.len==0);
}

void test_find_first_nul(void) {
    char buf[5] = {'a','b','\0','c','\0'};
    CHECK("FIND_FIRST_NUL_BYTE", FIND_FIRST_NUL_BYTE(buf,5)==&buf[2]);
    CHECK("FIND_FIRST_NUL_BYTE none", FIND_FIRST_NUL_BYTE(buf,2)==NULL);
}

void test_check_macros(void) {
    DECL_VARCHAR(v,5);
    strcpy(v.arr,"hi"); v.len=2;
    VARCHAR_CHECK_LEN(v);
    VARCHAR_CHECK_STR(v);
    VARCHAR_CHECK(v);
    CHECK("VARCHAR_CHECK ok",1);
}

void test_zsetlen(void) {
    DECL_VARCHAR(v,6);
    strcpy(v.arr,"hello");
    VARCHAR_ZSETLEN(v);
    CHECK("VARCHAR_ZSETLEN", v.len==5);
}

void test_setlenz(void) {
    DECL_VARCHAR(v,6);
    memcpy(v.arr,"abc",3); v.len=3;
    VARCHAR_SETLENZ(v);
    CHECK("VARCHAR_SETLENZ", v.arr[3]=='\0');
}

void test_copy(void) {
    DECL_VARCHAR(src,6); DECL_VARCHAR(dst,6);
    strcpy(src.arr,"abc"); src.len=3;
    VARCHAR_COPY(dst,src);
    CHECK("VARCHAR_COPY len", dst.len==3 && strcmp(dst.arr,"abc")==0);
}

void test_copy_in_out(void) {
    DECL_VARCHAR(v,6); char buf[6];
    VARCHAR_COPY_IN(v,"hey");
    CHECK("COPY_IN len", v.len==3 && strcmp(v.arr,"hey")==0);
    VARCHAR_COPY_OUT(buf,v);
    CHECK("COPY_OUT", strcmp(buf,"hey")==0);
}

int main(int argc,char **argv) {
    for(int i=1;i<argc;i++) if(!strcmp(argv[i],"--verbose")||!strcmp(argv[i],"-v")) verbose=1;

    test_init();
    test_find_first_nul();
    test_check_macros();
    test_zsetlen();
    test_setlenz();
    test_copy();
    test_copy_in_out();

    expect_abort(abort_check_len, "CHECK_LEN abort");
    expect_abort(abort_check_str, "CHECK_STR abort");
    expect_abort(abort_zsetlen_overflow, "ZSETLEN abort");
    expect_abort(abort_setlenz_overflow, "SETLENZ abort");
    expect_abort(abort_copy_small_dest, "COPY dest abort");
    expect_abort(abort_copy_in_overflow, "COPY_IN abort");
    expect_abort(abort_copy_out_small, "COPY_OUT abort");

    if(failures==0) {
        printf(verbose?"\nAll tests passed.\n":"\n");
    } else {
        printf("\n%d test(s) failed.\n", failures);
    }
    return failures;
}
