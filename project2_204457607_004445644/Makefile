CC = g++
CFLAGS = -Wall -Wextra -march=native -mtune=native -g -std=c++0x
OBJ = BFTP.o RDTP.o Printer.o Packet.o

%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

default: client server

client: client.o $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

server: server.o $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o client server project2_204457607_004445644.tar

dist:
	cd .. && tar -cf project2_204457607_004445644/project2_204457607_004445644.tar project2_204457607_004445644/README project2_204457607_004445644/Makefile project2_204457607_004445644/server.cpp project2_204457607_004445644/client.cpp project2_204457607_004445644/report.pdf project2_204457607_004445644/RDTP.cpp project2_204457607_004445644/RDTP.h project2_204457607_004445644/Printer.cpp project2_204457607_004445644/Printer.h project2_204457607_004445644/BFTP.cpp project2_204457607_004445644/BFTP.h project2_204457607_004445644/Packet.cpp project2_204457607_004445644/Packet.h project2_204457607_004445644/RDTPConnection.tpp
