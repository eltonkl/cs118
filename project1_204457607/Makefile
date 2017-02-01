CC = g++
CFLAGS = -Wall -Wextra -march=native -mtune=native -std=c++0x

default: webserver

webserver: webserver.cpp
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f *.o webserver project1_204457607.zip

dist:
	cd .. && zip project1_204457607/project1_204457607.zip project1_204457607/README project1_204457607/Makefile project1_204457607/webserver.cpp project1_204457607/report.pdf