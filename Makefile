CXXFLAGS:=-std=gnu++14 -Wall -MMD -MP


all: test 

clean:
	rm -f *~ *.o *.d test

-include *.d

SIMPLESOCKETS=comboaddress.o swrappers.o sclasses.o

test: test.o $(SIMPLESOCKETS)
	g++ -std=gnu++14 $^ -o $@


