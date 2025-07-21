PROGRAMS = test-varchar test-zvarchar test-fixed test-cstr

INC=../include

IV=${INC}/vsuite

CFLAGS = -Wall -Wextra -std=gnu99 -I${INC}

.PHONY: all test clean

all: $(PROGRAMS)

%: %.c
	gcc $(CFLAGS) -o $@ $<

test-varchar.ex:    test-varchar.c    ${IV}/varchar.h
test-zvarchar:   test-zvarchar.c   ${IV}/varchar.h  ${IV}/zvarchar.h
test-fixed:      test-fixed.c      ${IV}/varchar.h  ${IV}/fixed.h
test-cstr:       test-cstr.c       ${IV}/varchar.h  ${IV}/pstr.h

test: all
	@ for target in $(PROGRAMS) ; do \
            ( set -x && ./$${target} ) ; \
        done

clean:
	rm -f *.o $(PROGRAMS)
