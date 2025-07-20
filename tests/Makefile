PROGRAMS = test-v.exe test-zv.exe

INC=../include

IV=${INC}/vsuite

CFLAGS = -Wall -Wextra -std=c99 -I${INC}

.PHONY: all test clean

all: $(PROGRAMS)

%.exe: %.c
	gcc $(CFLAGS) -o $@ $<

test-v.exe:    test-v.c    ${IV}/v.h

test-zv.exe:   test-zv.c   ${IV}/v.h  ${IV}/zv.h

# test-vf.exe:   test-vf.c   ${IV}/v.h  ${IV}/vf.h
# test-fv.exe:   test-vf.c   ${IV}/v.h  ${IV}/fv.h
# test-vd.exe:   test-vd.c   ${IV}/v.h  ${IV}/vd.h
# test-dv.exe:   test-dv.c   ${IV}/v.h  ${IV}/dv.h

test: all
	mkdir -p log
	./test-v.exe   2>&1  | tee log/test-v.txt
	./test-zv.exe  2>&1  | tee log/test-zv.txt

clean:
	rm -f $(PROGRAMS) *.o
