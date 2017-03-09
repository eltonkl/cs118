#include <vector>
#include <chrono>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "RDTP.h"
#include "Packet.h"
#include "Printer.h"

using namespace std;
using namespace RDTP;
using namespace RDTP::_Internals;

namespace RDTP
{
	// Bytes
	const size_t Constants::MaxPacketSize = 1024;
	const size_t Constants::MaxSequenceNumber = 30720;
	const size_t Constants::WindowSize = 5120;
	const size_t Constants::HeaderSize = 8;
	// Milliseconds
	const size_t Constants::RetransmissionTimeoutValue = 500;

	const size_t Constants::InitialSlowStartThreshold = 15360;
	const size_t Constants::InitialCongestionWindowSize = 1024;

    // Three way handshake
    RDTPIOStream::RDTPIOStream(const int sockfd, const struct sockaddr_in& cli_addr) :
        _sockfd(sockfd), _cli_addr(cli_addr), _cli_len(sizeof(cli_addr)), _printer(cout)
    {

    }
    
    // Send data
	RDTPIOStream& RDTPIOStream::operator<<(const vector<unsigned char>& data)
    {
		(void)data;
		return *this;
    }

    // Receive data
    RDTPIOStream& RDTPIOStream::operator>>(vector<unsigned char>& data)
    {
		(void)data;
		return *this;
    }
}