CC = g++
CFLAGS = -Wall -Wextra -march=native -mtune=native -std=c++11

default: webserver

webserver: webserver.c
	$(CC) $(CFLAGS) $^ -o $@
