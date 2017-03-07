#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <cassert>

#include "RDTP.h"
#include "Printer.h"

using namespace std;
using namespace RDTP;

void sigchld_handler(int s)
{
    (void)s;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// Print error message and then exit
void error(string msg)
{
    perror(msg.c_str());
    exit(1);
}

void respondToClient(int sockfd, struct sockaddr_in* cli_addr, socklen_t cli_len, const Packet& packet);

void test()
{
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
    int sockfd, portno, pid;
    socklen_t cli_len;
    struct sockaddr_in serv_addr, cli_addr;
    struct sigaction sa;
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

    cli_len = sizeof(cli_addr);

    // Kill zombie processes
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, nullptr) == -1)
        error("sigaction");

    while (true)
    {
        len = recvfrom(sockfd, buf, Constants::MaxPacketSize, 0, (struct sockaddr*)&cli_addr, &cli_len);

        // Fork to handle incoming requests
        pid = fork();
        if (pid < 0)
            error("ERROR on fork");

        // Child process: handle request, close socket
        if (pid == 0)
        {
            if (len > 0)
            {
                Packet packet = Packet::FromRawData(buf, len);
                Printer printer(cout);
                printer.PrintInformation(ApplicationType::Server, packet, false);
                respondToClient(sockfd, &cli_addr, cli_len, packet);
                close(sockfd);
                exit(0);
            }
        }
    }

    return 0;
}

void respondToClient(int sockfd, struct sockaddr_in* cli_addr, socklen_t cli_len, const Packet& packet)
{
    (void)sockfd;
    (void)cli_addr;
    (void)cli_len;
    (void)packet;
    //sendto(sockfd, buf, buflen, (struct sockaddr *)cli_addr, cli_len);
}