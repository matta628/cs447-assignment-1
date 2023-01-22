CFLAGS = -g -Wall -Wextra -pedantic -O2

all: integrate

integrate: integrate.o
	g++ $(CFLAGS) integrate.o -o integrate -pthread
integrate.o: integrate.cpp
	g++ $(CFLAGS) -c integrate.cpp

clean:
	rm integrate.o integrate
pi: all
	./integrate -3.141592 3.141592 10000000 1
memcheck: all
	valgrind ./integrate -1 1 1000000 1
memcheck++: all
	valgrind --leak-check=full ./integrate -1 1 1000000 1
