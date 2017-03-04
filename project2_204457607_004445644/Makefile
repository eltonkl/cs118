CC = g++
CFLAGS = -Wall -Wextra -march=native -mtune=native -std=c++0x

default: client server

# forgive me, for I have sinned and don't feel like having a separate object file creation step and cannot reap the benefits of not having to recompile every cpp file every time i type make

client: client.cpp Printer.cpp Packet.cpp
	$(CC) $(CFLAGS) $^ -o $@

server: server.cpp Printer.cpp Packet.cpp
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f *.o client server project2_204457607_004445644.tar

dist:
	cd .. && tar -cf project2_204457607_004445644/project2_204457607_004445644.tar project2_204457607_004445644/README project2_204457607_004445644/Makefile project2_204457607_004445644/server.cpp project2_204457607_004445644/client.cpp project2_204457607_004445644/report.pdf project2_204457607_004445644/rdtp.cpp project2_204457607_004445644/rdtp.h project2_204457607_004445644/print.cpp project2_204457607_004445644/print.h
