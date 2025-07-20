PROGRAMS = test-suite.exe

.SUFFIXES: .c .exe

all: $(PROGRAMS)

test-suite.exe: test-suite.c varchar-check.h
	gcc -Wall -Wextra -std=c99 -o $@ $<

clean:
	rm -f *.exe *.o *.lis *.log
