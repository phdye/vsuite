PROGRAMS = test-varchar test-zvarchar test-fixed test-pstr test-logfile

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
test-pstr:       test-pstr.c       ${IV}/varchar.h  ${IV}/pstr.h
test-logfile:    test-logfile.c    ${INC}/varchar-logFile.h ${IV}/zvarchar.h ${IV}/varchar.h

PYTEST = python3 -m unittest -v test_better_varchar.py

PYTEST = python3 -m unittest -v test_better_varchar.py

test: all
	@for target in $(PROGRAMS) ; do \
	    ( set -x && ./$${target} ) ; \
	done
	@$(PYTEST)

vtest: all
	@ for target in $(PROGRAMS) ; do \
            ( set -x && ./$${target} --verbose ) ; \
        done

clean:
	rm -f *.o $(PROGRAMS)
