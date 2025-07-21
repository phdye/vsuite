TARGETS = tests

.PHONY: all test clean

all test :
	for target in $(TARGETS) ; do ( cd $${target} && make $@ ) ; done

clean :
	echo y | clean
	for target in $(TARGETS) ; do ( cd $${target} && make $@ ) ; done
