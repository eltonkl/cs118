CXX = g++
CPPFLAGS = -Wall -Wextra -march=native -mtune=native -g -std=c++0x
BUILD_DIR = ./build
COMMON_CPP = Packet.cpp Printer.cpp RDTP.cpp BFTP.cpp
CLIENT_CPP = client.cpp
SERVER_CPP = server.cpp
COMMON_OBJ = $(COMMON_CPP:%.cpp=$(BUILD_DIR)/%.o)
CLIENT_OBJ = $(CLIENT_CPP:%.cpp=$(BUILD_DIR)/%.o)
SERVER_OBJ = $(SERVER_CPP:%.cpp=$(BUILD_DIR)/%.o)
COMMON_DEP = $(COMMON_OBJ:%.o=%.d)
CLIENT_DEP = $(CLIENT_OBJ:%.o=%.d)
SERVER_DEP = $(SERVER_OBJ:%.o=%.d)

default: build_dir client server

build_dir: $(BUILD_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

client: $(CLIENT_OBJ) $(COMMON_OBJ)
	$(CXX) $(CPPFLAGS) $^ -o $@

server: $(SERVER_OBJ) $(COMMON_OBJ)
	$(CXX) $(CPPFLAGS) $^ -o $@

-include $(COMMON_DEP) $(CLIENT_DEP) $(SERVER_DEP)

$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CPPFLAGS) -MMD -c $< -o $@

.PHONY: clean
clean:
	rm -f $(COMMON_OBJ) $(CLIENT_OBJ) $(SERVER_OBJ) $(COMMON_DEP) $(CLIENT_DEP) $(SERVER_DEP) client server project2_204457607_004445644.tar
	rmdir $(BUILD_DIR)

dist:
	cd .. && tar -cf project2_204457607_004445644/project2_204457607_004445644.tar project2_204457607_004445644/README project2_204457607_004445644/Makefile project2_204457607_004445644/server.cpp project2_204457607_004445644/client.cpp project2_204457607_004445644/report.pdf project2_204457607_004445644/RDTP.cpp project2_204457607_004445644/RDTP.h project2_204457607_004445644/Printer.cpp project2_204457607_004445644/Printer.h project2_204457607_004445644/BFTP.cpp project2_204457607_004445644/BFTP.h project2_204457607_004445644/Packet.cpp project2_204457607_004445644/Packet.h project2_204457607_004445644/RDTPConnection.tpp
