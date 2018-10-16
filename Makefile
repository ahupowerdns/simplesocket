CXXFLAGS:=-std=gnu++14 -Wall -MMD -MP -Iext/fmt-5.2.1/include


all: test 

clean:
	rm -f *~ *.o *.d test

-include *.d

SIMPLESOCKETS=comboaddress.o swrappers.o sclasses.o ext/fmt-5.2.1/src/format.o

test: test.o $(SIMPLESOCKETS) 
	g++ -std=gnu++14 $^ -o $@


