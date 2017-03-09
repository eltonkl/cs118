#include <vector>
#include <chrono>
#include <netinet/in.h>
#include <sys/socket.h>

#include "RDTP.h"
#include "Packet.h"
#include "Printer.h"

using namespace std;
using namespace RDTP::_Internals;

namespace RTDP
{
    // Three way handshake
    RDTPIOStream::RDTPIOStream(const int sockfd, const struct sockaddr_in& cli_addr) :
        _sockfd(sockfd), _cli_addr(cli_addr), _cli_len(sizeof(cli_addr)), _printer(cout)
    {

    }
    
    // Send data
    RDTPIOStream& RDTPIOStream:operator<<(const std::vector<unsigned char>& data)
    {

    }

    // Receive data
    RDTPIOStream& operator>>(std::vector<unsigned char>& data)
    {

    }
}