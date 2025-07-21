PROGRAMS = test-v.exe test-zv.exe test-vf.exe test-fv.exe test-vd.exe test-dv.exe

INC=../include

IV=${INC}/vsuite

CFLAGS = -Wall -Wextra -std=gnu99 -I${INC}

.PHONY: all test clean

all: $(PROGRAMS)

%.exe: %.c
	gcc $(CFLAGS) -o $@ $<

test-v.exe:    test-v.c    ${IV}/varchar.h
test-zv.exe:   test-zv.c   ${IV}/varchar.h  ${IV}/zvarchar.h
test-vf.exe:   test-vf.c   ${IV}/varchar.h  ${IV}/fixed.h
test-fv.exe:   test-fv.c   ${IV}/varchar.h  ${IV}/fixed.h
test-vd.exe:   test-vd.c   ${IV}/varchar.h  ${IV}/cstr.h
test-dv.exe:   test-dv.c   ${IV}/varchar.h  ${IV}/cstr.h

test: all
	./test-v.exe
	./test-zv.exe
	./test-vf.exe
	./test-fv.exe
	./test-vd.exe
	./test-dv.exe

clean:
	rm -f $(PROGRAMS) *.o
