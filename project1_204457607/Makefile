CC = g++
CFLAGS = -Wall -Wextra -march=native -mtune=native -std=c++11

default: webserver

webserver: webserver.cpp
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f *.o webserver
