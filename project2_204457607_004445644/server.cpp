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
#include <iterator>
#include <iomanip>

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

// void test()
// {
//     using namespace RDTP::_Internals;
//     vector<char> data = { 'l', 'e', 'l' };
//     Packet packet(PacketType::SYN, 5, 23, 93, data.data(), data.size());
//     vector<char> rawData = packet.GetRawData();
//     Packet packet2 = Packet::FromRawData(rawData.data(), rawData.size());
//     assert(packet.GetPacketType() == packet2.GetPacketType());
//     assert(packet.GetSequenceNumber() == packet2.GetSequenceNumber());
//     assert(packet.GetAcknowledgeNumber() == packet2.GetAcknowledgeNumber());
//     assert(packet.GetWindowSize() == packet2.GetWindowSize());
//     assert(packet.GetData() == packet2.GetData());

//     Printer printer(cout);
//     printer.PrintInformation(ApplicationType::Server, packet, false);
//     printer.PrintInformation(ApplicationType::Client, packet, false);

//     size_t x = SIZE_MAX;
//     stringstream ss;
//     ss << setfill('0') << setw(20) << x;
//     x = 0;
//     ss >> x;
//     cout << x << endl;
//     string a = ss.str();
//     stringstream ss2;
//     x = 0;
//     ss2 << '1' << a;
//     cout << string(istream_iterator<char>(ss2), istream_iterator<char>()) << endl;
//     ss2 = stringstream();
//     ss2 << a;
//     ss2 >> x;
//     cout << x << endl;

//     int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
//     struct timeval tv;
//     socklen_t tv_len;
//     cout << getsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, &tv_len) << endl;
//     close(sockfd);
//     assert(false);
// }

int main(int argc, char** argv)
{
    // test();
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
        cout << "Initiating RDTP connection as server." << endl;
        RDTPConnection rc(ApplicationType::Server, sockfd);
        if (!rc.IsConnectionEstablished())
            continue;
        BFTPSession bs(rc);

        string filename = bs.ReceiveFilename();
        cout << "Filename requested: " << filename << endl;
        
        ifstream ifs(filename, ios::binary);
        struct stat attr;
        int res = stat(filename.c_str(), &attr);
        
        if (ifs.fail() || (res == 0 && S_ISDIR(attr.st_mode))) // If the file couldn't be opened or is a directory, use the failure response (404 Not Found)
        {
            cout << "File not found." << endl;
            bs.NotifyFileNotFound();
        }
        else
        {
            cout << "File found. Sending." << endl;
            std::streampos beg, end;
            beg = ifs.tellg();
            ifs.seekg(0, std::ios::end);
            end = ifs.tellg();
            ifs.seekg(0, std::ios::beg);

            bs.SendFile(ifs, end - beg);
        }

        cout << "Done serving client." << endl;
    }

    close(sockfd);
    return 0;
}