CXXFLAGS:=-std=gnu++14 -Wall -MMD -MP


all: test

clean:
	rm -f *~ *.o *.d test

-include *.d

test: test.o comboaddress.o swrappers.o
	g++ -std=gnu++14 $^ -o $@

