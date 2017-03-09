#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <cassert>

#include "RDTP.h"

using namespace std;
using namespace std::chrono;
using namespace RDTP;

// Print error message and then exit
void error(string msg)
{
    perror(msg.c_str());
    exit(1);
}

//void respondToClient(int sockfd, struct sockaddr_in* cli_addr, socklen_t cli_len, const Packet& packet);

void test()
{
    using namespace RDTP::_Internals;
    vector<unsigned char> data = { 'l', 'e', 'l' };
    Packet packet(PacketType::SYN, 5, 23, 93, data.data(), data.size());
    vector<unsigned char> rawData = packet.GetRawData();
    Packet packet2 = Packet::FromRawData(rawData.data(), rawData.size());
    assert(packet.GetPacketType() == packet2.GetPacketType());
    assert(packet.GetSequenceNumber() == packet2.GetSequenceNumber());
    assert(packet.GetAcknowledgeNumber() == packet2.GetAcknowledgeNumber());
    assert(packet.GetWindowSize() == packet2.GetWindowSize());
    assert(packet.GetData() == packet2.GetData());

    Printer printer(cout);
    printer.PrintInformation(ApplicationType::Server, packet, false);
    printer.PrintInformation(ApplicationType::Client, packet, false);
}

int main(int argc, char** argv)
{
    (void) test;
    int sockfd, portno;
    struct sockaddr_in serv_addr, cli_addr;
    unsigned char buf[Constants::MaxPacketSize];
    int len;

    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    memset(&serv_addr, '\0', sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(portno);

    if (::bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    bool servingFile = false;

    
    /*while (true)
    {
        milliseconds ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

        if (!servingFile) // TCP-like 3-way handshake + receive the filename from the client
        {
            
        }
        else
        {
            len = recvfrom(sockfd, buf, Constants::MaxPacketSize, 0, (struct sockaddr*)&cli_addr, &cli_len);
        
            if (len > 0)
            {
                Packet packet = Packet::FromRawData(buf, len);
                printer.PrintInformation(ApplicationType::Server, packet, false);
                respondToClient(sockfd, &cli_addr, cli_len, packet);
            }
        }
    }*/

    return 0;
}

/*void respondToClient(int sockfd, struct sockaddr_in* cli_addr, socklen_t cli_len, const Packet& packet)
{
    (void)sockfd;
    (void)cli_addr;
    (void)cli_len;
    (void)packet;
    //sendto(sockfd, buf, buflen, (struct sockaddr *)cli_addr, cli_len);
}*/