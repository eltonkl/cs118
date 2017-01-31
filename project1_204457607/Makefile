CC = g++
CFLAGS = -Wall -Wextra -march=native -mtune=native -std=c++0x

default: webserver

webserver: webserver.cpp
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f *.o webserver
