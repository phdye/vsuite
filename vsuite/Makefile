PROGRAMS = test-v.exe test-zv.exe

CFLAGS = -Wall -Wextra -std=c99 -I..

all: $(PROGRAMS)

%.exe: %.c v.h zv.h
	gcc $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGRAMS) *.o *.lis *.log
