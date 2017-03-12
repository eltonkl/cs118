#include <vector>
#include <chrono>
#include <iterator>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "RDTP.h"
#include "Packet.h"
#include "Printer.h"

using namespace std;
using namespace std::chrono;
using namespace RDTP;
using namespace RDTP::_Internals;

namespace RDTP
{
	// Print error message and then exit
	void _Error(string msg)
	{
		perror(msg.c_str());
	}

	bool _setTimeout(int sockfd, int microsec)
	{
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = microsec;

		return setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0;
	}

	// Bytes
	const size_t Constants::MaxPacketSize = 1024;
	const size_t Constants::MaxSequenceNumber = 30720;
	const size_t Constants::WindowSize = 5120;
	const size_t Constants::HeaderSize = 8;
	// Milliseconds
	const size_t Constants::RetransmissionTimeoutValue = 500;
	const size_t Constants::RetransmissionTimeoutValue_us = Constants::RetransmissionTimeoutValue * 1000;

	const size_t Constants::InitialSlowStartThreshold = 15360;
	const size_t Constants::InitialCongestionWindowSize = 1024;

    // Three way handshake
    RDTPConnection::RDTPConnection(ApplicationType type, const int sockfd) :
        _sockfd(sockfd), _cli_len(sizeof(_cli_addr)), _printer(cout)
	{
		if (type == ApplicationType::Server)
			ReceiveHandshake();
		else //if (type == ApplicationType::Client)
			InitiateHandshake();
		_established = true;
    }

	bool RDTPConnection::IsConnectionEstablished() const
	{
		return _established;
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

	// Server
	void RDTPConnection::ReceiveHandshake()
	{
		ssize_t len;
		char buf[Constants::MaxPacketSize];

		// LISTEN
		len = recvfrom(_sockfd, buf, Constants::MaxPacketSize, 0, (struct sockaddr*)&_cli_addr, &_cli_len);
		if (len > 0)
		{
			Packet packet = Packet::FromRawData(buf, len);
			_printer.PrintInformation(ApplicationType::Server, packet, false);
		
			if (packet.GetPacketType() == PacketType::SYN)
				_sendBase = packet.GetSequenceNumber();
			else
			{
				cerr << "Expected SYN packet, got something else" << endl;
				goto failure;
			}
		}
		else
		{
			cerr << "recvfrom failed" << endl;
			goto perror_then_failure;
		}

		// SYN_RCVD
		{
			if (!_setTimeout(_sockfd, Constants::RetransmissionTimeoutValue_us))
			{
				cerr << "Failed to set receive timeout value" << endl;
				goto perror_then_failure;
			}

			_nextSeqNum = 0;
			_sendBase += 1;
			Packet packet = Packet(PacketType::SYNACK, _nextSeqNum, _sendBase, Constants::WindowSize, nullptr, 0);
			bool retransmit = false;
	
			_printer.PrintInformation(ApplicationType::Server, packet, retransmit);
			sendto(_sockfd, packet.GetRawData().data(), packet.GetRawDataSize(), 0, (struct sockaddr*)&_cli_addr, _cli_len);
			
			len = recvfrom(_sockfd, buf, Constants::MaxPacketSize, 0, (struct sockaddr*)&_cli_addr, &_cli_len);
			if (len > 0)
			{
				Packet packet = Packet::FromRawData(buf, len);
				_printer.PrintInformation(ApplicationType::Server, packet, false);

				if (packet.GetPacketType() != PacketType::ACK)
				{
					cerr << "Received unexpected packet, expected ACK" << endl;
					goto failure;
				}
			}
			else
			{
				// http://stackoverflow.com/questions/16259774/what-if-a-tcp-handshake-segment-is-lost
			}
		}
		// ESTABLISHED
		return;
	perror_then_failure:
		_Error("RDTP handshake failed");
	failure:
		_established = false;
	}

	// Client
	void RDTPConnection::InitiateHandshake()
	{
		ssize_t len;
		char buf[Constants::MaxPacketSize];

		bool retransmit = false;
		_sendBase = 0;
		_nextSeqNum = 0;
		Packet packet = Packet(PacketType::SYN, _nextSeqNum, _sendBase, Constants::WindowSize, nullptr, 0);
		
		if (!_setTimeout(_sockfd, Constants::RetransmissionTimeoutValue_us))
		{
			cerr << "Failed to set receive timeout value" << endl;
			goto perror_then_failure;
		}

		while (true)
		{
			// milliseconds ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
			_printer.PrintInformation(ApplicationType::Client, packet, retransmit);
			write(_sockfd, packet.GetRawData().data(), packet.GetRawDataSize());
			
			// SYN_SENT
			len = recv(_sockfd, buf, Constants::MaxPacketSize, 0);
			if (len < 0) // Response was not received in 500 ms, retry
			{
				retransmit = true;
				continue;
			}

			Packet packet = Packet::FromRawData(buf, len);
			_printer.PrintInformation(ApplicationType::Client, packet, false);

			if (packet.GetPacketType() == PacketType::SYNACK)
			{
				_nextSeqNum = packet.GetAcknowledgeNumber();
				_sendBase = packet.GetSequenceNumber();
				break;
			}
			else
			{
				cerr << "Received unexpected packet, expected SYNACK" << endl;
				goto failure;
			}
		}

		// SYNACK received, send ACK
		{
			_sendBase += 1;

			Packet packet2 = Packet(PacketType::SYN, _nextSeqNum, _sendBase, Constants::WindowSize, nullptr, 0);
			_printer.PrintInformation(ApplicationType::Client, packet2, retransmit);
			
			write(_sockfd, packet2.GetRawData().data(), packet2.GetRawDataSize());
			// Hope it sends: http://stackoverflow.com/questions/16259774/what-if-a-tcp-handshake-segment-is-lost
		}

		// ESTABLISHED
		return;
	perror_then_failure:
		_Error("RDTP handshake failed");
	failure:
		_established = false;
	}

	// Server
	void RDTPConnection::ReceiveFinish()
	{

	}

	// Client
	void RDTPConnection::SendFinish()
	{

	}
}