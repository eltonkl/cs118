CC = g++
CFLAGS = -Wall -Wextra -march=native -mtune=native -g -std=c++0x

default: client server

client: client.cpp BFTP.cpp RDTP.cpp Printer.cpp Packet.cpp
	$(CC) $(CFLAGS) $^ -o $@

server: server.cpp BFTP.cpp RDTP.cpp Printer.cpp Packet.cpp
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f *.o client server project2_204457607_004445644.tar

dist:
	cd .. && tar -cf project2_204457607_004445644/project2_204457607_004445644.tar project2_204457607_004445644/README project2_204457607_004445644/Makefile project2_204457607_004445644/server.cpp project2_204457607_004445644/client.cpp project2_204457607_004445644/report.pdf project2_204457607_004445644/rdtp.cpp project2_204457607_004445644/rdtp.h project2_204457607_004445644/print.cpp project2_204457607_004445644/print.h
