#include <vector>
#include <chrono>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <iterator>

#include "RDTP.h"
#include "Packet.h"
#include "Printer.h"

using namespace std;
using namespace std::chrono;
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
    RDTPConnection::RDTPConnection(ApplicationType type, const int sockfd) :
        _sockfd(sockfd), _cli_len(sizeof(_cli_addr)), _printer(cout)
    {
		ssize_t len;
		char buf[Constants::MaxPacketSize];

		if (type == ApplicationType::Server)
		{
			while (true)
			{
				milliseconds ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

				len = recvfrom(sockfd, buf, Constants::MaxPacketSize, 0, (struct sockaddr*)&_cli_addr, &_cli_len);
				if (len > 0)
				{
					Packet packet = Packet::FromRawData(buf, len);
					_printer.PrintInformation(ApplicationType::Server, packet, false);
				}
			}
		}
		else
		{
			while (true)
			{
				milliseconds ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

				len = recvfrom(sockfd, buf, Constants::MaxPacketSize, 0, (struct sockaddr*)&_cli_addr, &_cli_len);
				if (len > 0)
				{
					Packet packet = Packet::FromRawData(buf, len);
					_printer.PrintInformation(ApplicationType::Server, packet, false);
				}
			}
		}
    }

	// Write data
	RDTPConnection& RDTPConnection::operator<<(std::basic_istream<char>& is)
    {
		Write(istream_iterator<char>(is), istream_iterator<char>());
		return *this;
    }

    // Read data
    void RDTPConnection::Read(std::basic_ostream<char>& os, size_t count)
    {
		ostream_iterator<char> osi(os);
		(void)osi;
		(void)count;
    }
}