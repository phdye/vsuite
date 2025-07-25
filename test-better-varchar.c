#include <string.h>
#include "vsuite/varchar.h"

int main(void) {
    VARCHAR(src, 8);
    VARCHAR(dst, 8);

    strcpy((char*)dst.arr, (char*)src.arr);
    dst.len = strlen(dst.arr);
    dst.arr[dst.len] = '\0';

    return dst.len;
}
