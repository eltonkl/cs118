#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <cassert>
#include <sstream>
#include <string>

#include "RDTP.h"
#include "BFTP.h"

using namespace std;
using namespace RDTP;
using namespace BFTP;

// Print error message and then exit
void error(string msg)
{
    perror(msg.c_str());
    exit(1);
}

void test()
{
    using namespace RDTP::_Internals;
    vector<char> data = { 'l', 'e', 'l' };
    Packet packet(PacketType::SYN, 5, 23, 93, data.data(), data.size());
    vector<char> rawData = packet.GetRawData();
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
    test();
    int sockfd, portno;
    struct sockaddr_in serv_addr;

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

    while (true)
    {
        RDTPConnection rc(ApplicationType::Server, sockfd);
        BFTPSession bs(rc);

        string filename = bs.ReceiveFilename();

        cout << "Filename requested: " << filename;
        
        ifstream ifs(filename, ios::binary);
        struct stat attr;
        int res = stat(filename.c_str(), &attr);
        
        if (ifs.fail() || (res == 0 && S_ISDIR(attr.st_mode))) // If the file couldn't be opened or is a directory, use the failure response (404 Not Found)
        {
            bs.NotifyFileNotFound();
        }
        else
        {
            std::streampos beg, end;
            beg = ifs.tellg();
            ifs.seekg(0, std::ios::end);
            end = ifs.tellg();
            ifs.seekg(0, std::ios::beg);

            bs.SendFile(ifs, beg - end);
        }
    }

    close(sockfd);
    return 0;
}