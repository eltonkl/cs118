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
	namespace _Internals
    {
		// Print error message and then exit
		void _Error(string msg)
		{
			perror(msg.c_str());
		}
		
		bool _setTimeout(int sockfd, int sec, int microsec)
		{
			struct timeval tv;
			tv.tv_sec = sec;
			tv.tv_usec = microsec;

			bool result = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0;
			if (!result)
				cerr << "Failed to set receive timeout value." << endl;
			return result;
		}
	}

	// Bytes
	const uint32_t Constants::MaxPacketSize = 1024;
	const uint32_t Constants::MaxSequenceNumber = 30720;
	const uint32_t Constants::WindowSize = 5120;
	const uint32_t Constants::HeaderSize = 5;
	// Milliseconds
	const uint32_t Constants::RetransmissionTimeoutValue = 500;
	const uint32_t Constants::RetransmissionTimeoutValue_us = Constants::RetransmissionTimeoutValue * 1000;
	// Seconds
	const uint32_t Constants::MaximumFinishRetryTimeValue = 1; // (2 * RTO)

	const uint32_t Constants::InitialSlowStartThreshold = 15360;
	const uint32_t Constants::InitialCongestionWindowSize = 1024;

    // Three way handshake
    RDTPConnection::RDTPConnection(ApplicationType type, const int sockfd) :
        _sockfd(sockfd), _cli_len(sizeof(_cli_addr)), _printer(cout), _type(type)
	{
		_established = true;
		_firstDataPacket = nullptr;
		if (_type == ApplicationType::Server)
			ReceiveHandshake();
		else //if (_type == ApplicationType::Client)
			InitiateHandshake();
    }

	RDTPConnection::~RDTPConnection()
	{
		if (_firstDataPacket)
			delete _firstDataPacket;
		if (_type == ApplicationType::Server)
			SendFinish();
		else //if (_type == ApplicationType::Client)
			ReceiveFinish();
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

		size_t read = 0;
		ssize_t len;
		char buf[Constants::MaxPacketSize];

		if (!_setTimeout(_sockfd, 0, 0))
			goto perror_then_failure;
		while (read < count)
		{
			Packet packet;
			if (_firstDataPacket)
			{
				packet = *_firstDataPacket;
				delete _firstDataPacket;
				_firstDataPacket = nullptr;
			}
			else
			{
				len = recvfrom(_sockfd, buf, Constants::MaxPacketSize, 0, (struct sockaddr*)&_cli_addr, &_cli_len);
				packet = Packet::FromRawData(buf, len);
				_printer.PrintInformation(_type, packet, false, true);
			}
			
			bool rotatedFirst = false;
			bool rotatedSecond = false;
			uint64_t realRcvBase = _rcvBase % Constants::MaxSequenceNumber;

			if (packet.GetPacketType() != PacketType::NONE)
				continue;
			if ((int64_t)(_rcvBase % Constants::MaxSequenceNumber) - Constants::WindowSize < 0)
				rotatedFirst = true;
			if ((realRcvBase + Constants::WindowSize) > Constants::MaxSequenceNumber)
				rotatedSecond = true;

			if ((rotatedFirst && ((((_rcvBase - Constants::WindowSize) % Constants::MaxSequenceNumber <= packet.GetNumber() && packet.GetNumber() <= Constants::MaxSequenceNumber)) || (packet.GetNumber() <= (_rcvBase % Constants::MaxSequenceNumber) - 1)))
				|| (!rotatedFirst && (_rcvBase - Constants::WindowSize) <= packet.GetNumber() && packet.GetNumber() <= _rcvBase - 1))
			{
				Packet ack = Packet(PacketType::ACK, packet.GetNumber(), Constants::WindowSize, nullptr, 0);
				_printer.PrintInformation(_type, ack, false, false);
				sendto(_sockfd, ack.GetRawData().data(), ack.GetRawDataSize(), 0, (struct sockaddr*)&_cli_addr, _cli_len);
			}
			else if ((rotatedSecond && ((_rcvBase <= packet.GetNumber() && packet.GetNumber() <= Constants::MaxSequenceNumber) || packet.GetNumber() <= (_rcvBase + Constants::WindowSize - 1) % Constants::MaxSequenceNumber))
			     || (!rotatedSecond && _rcvBase <= packet.GetNumber() && packet.GetNumber() <= _rcvBase + Constants::WindowSize - 1))
			{
				Packet ack = Packet(PacketType::ACK, packet.GetNumber(), Constants::WindowSize, nullptr, 0);
				_printer.PrintInformation(_type, ack, false, false);
				sendto(_sockfd, ack.GetRawData().data(), ack.GetRawDataSize(), 0, (struct sockaddr*)&_cli_addr, _cli_len);

				auto it = _receivedPackets.begin();
				while (it != _receivedPackets.end())
				{
					if (it->GetNumber() > packet.GetNumber())
						break;
					it++;
				}
				_receivedPackets.insert(it, packet);

				while (_rcvBase % Constants::MaxSequenceNumber == _receivedPackets.front().GetNumber())
				{
					auto data = _receivedPackets.front().GetData();
					copy(data.begin(), data.end(), osi);
					read += data.size();
					_rcvBase += data.size();
					_receivedPackets.pop_front();
				}
			}
			else
			{
				// Do nothing
			}
		}
		return;
	perror_then_failure:
		_Error("RDTP Read failed");
    }

	// HERE BE DRAGONS
	// DON'T SCROLL PAST THIS PART OF THE FILE, OR YOU MIGHT LOSE YOUR SANITY

	// Really should have refactored code into helper functions, but code quality isn't part of the grade

	// Server
	void RDTPConnection::ReceiveHandshake()
	{
		ssize_t len;
		char buf[Constants::MaxPacketSize];
		_nextSeqNum = 0;
		_sendBase = 0;

		// LISTEN
		if (!_setTimeout(_sockfd, 0, 0))
			goto perror_then_failure;
		len = recvfrom(_sockfd, buf, Constants::MaxPacketSize, 0, (struct sockaddr*)&_cli_addr, &_cli_len);
		if (len > 0)
		{
			Packet packet = Packet::FromRawData(buf, len);
			_printer.PrintInformation(ApplicationType::Server, packet, false, true);
		
			if (packet.GetPacketType() == PacketType::SYN)
				_rcvBase = packet.GetNumber() + 1;
			else
			{
				if (packet.GetPacketType() == PacketType::FIN)
				{
					cerr << "Received unexpected FIN packet, sending ACK and giving up." << endl;
					Packet packet2(PacketType::ACK, packet.GetNumber(), Constants::WindowSize, nullptr, 0);
					sendto(_sockfd, packet2.GetRawData().data(), packet2.GetDataSize(), 0, (struct sockaddr*)&_cli_addr, _cli_len);
				}
				else
					cerr << "Expected SYN packet, got something else." << endl;
				goto failure;
			}
		}
		else
		{
			cerr << "recvfrom failed." << endl;
			goto perror_then_failure;
		}

		// SYN_RCVD
		{
			if (!_setTimeout(_sockfd, 0, Constants::RetransmissionTimeoutValue_us))
				goto perror_then_failure;

			Packet packet = Packet(PacketType::SYNACK, _nextSeqNum, Constants::WindowSize, nullptr, 0);
			bool retransmit = false;

			while (true)
			{
				_printer.PrintInformation(ApplicationType::Server, packet, retransmit, false);
				sendto(_sockfd, packet.GetRawData().data(), packet.GetRawDataSize(), 0, (struct sockaddr*)&_cli_addr, _cli_len);

				len = recvfrom(_sockfd, buf, Constants::MaxPacketSize, 0, (struct sockaddr*)&_cli_addr, &_cli_len);
				if (len > 0)
				{
					Packet packet = Packet::FromRawData(buf, len);
					_printer.PrintInformation(ApplicationType::Server, packet, false, true);

					if (packet.GetPacketType() == PacketType::SYN)
					{
						// Resend SYNACK
						retransmit = true;
						continue;
					}
					else if (packet.GetPacketType() != PacketType::ACK)
					{
						// Forward this packet to Read
						_firstDataPacket = new Packet(packet);
						break;
					}
					else
						break;
				}
				else
					retransmit = true;
			}
		}
		// ESTABLISHED
		_sendBase += 1;
		_nextSeqNum += 1;
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
		Packet packet = Packet(PacketType::SYN, _nextSeqNum, Constants::WindowSize, nullptr, 0);
		
		if (!_setTimeout(_sockfd, 0, Constants::RetransmissionTimeoutValue_us))
			goto perror_then_failure;

		while (true)
		{
			_printer.PrintInformation(ApplicationType::Client, packet, retransmit, false);
			write(_sockfd, packet.GetRawData().data(), packet.GetRawDataSize());
			
			// SYN_SENT
			len = recv(_sockfd, buf, Constants::MaxPacketSize, 0);
			if (len <= 0) // Response was not received in 500 ms, retry
			{
				retransmit = true;
				continue;
			}

			Packet packet = Packet::FromRawData(buf, len);
			_printer.PrintInformation(ApplicationType::Client, packet, false, true);

			if (packet.GetPacketType() == PacketType::SYNACK)
			{
				_rcvBase = packet.GetNumber() + 1;
				break;
			}
			else
			{
				cerr << "Received unexpected packet, expected SYNACK." << endl;
				goto failure;
			}
		}

		// SYNACK received, send ACK
		{
			_sendBase += 1;
			_nextSeqNum += 1;

			Packet packet2 = Packet(PacketType::ACK, _nextSeqNum, Constants::WindowSize, nullptr, 0);
			_printer.PrintInformation(ApplicationType::Client, packet2, false, false);
			
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

	// Client
	void RDTPConnection::ReceiveFinish()
	{
		ssize_t len;
		char buf[Constants::MaxPacketSize];

		if (!_setTimeout(_sockfd, 0, 0))
			goto perror_then_failure;

		while (true)
		{
			len = recvfrom(_sockfd, buf, Constants::MaxPacketSize, 0, (struct sockaddr*)&_cli_addr, &_cli_len);
			if (len > 0)
			{
				Packet packet = Packet::FromRawData(buf, len);
				_printer.PrintInformation(ApplicationType::Client, packet, false, true);

				if (packet.GetPacketType() == PacketType::FIN)
					break;
				else
				{
					cerr << "Expected FIN packet, got something else." << endl;
					continue;
				}
			}
			else
			{
				cerr << "recvfrom failed." << endl;
				goto failure;
			}
		}

		{
			Packet packet2 = Packet(PacketType::ACK, _rcvBase, Constants::WindowSize, nullptr, 0);

			_printer.PrintInformation(ApplicationType::Client, packet2, false, false);
			sendto(_sockfd, packet2.GetRawData().data(), packet2.GetRawDataSize(), 0, (struct sockaddr*)&_cli_addr, _cli_len);
		}

		// CLOSE_WAIT + LAST_ACK
		{
			_rcvBase += 1;
			Packet packet3 = Packet(PacketType::FIN, _nextSeqNum, Constants::WindowSize, nullptr, 0);
			_nextSeqNum += 1;
			bool retransmit = false;
			seconds start = duration_cast<seconds>(system_clock::now().time_since_epoch());

			if (!_setTimeout(_sockfd, 0, Constants::RetransmissionTimeoutValue_us))
				goto perror_then_failure;

			while (true)
			{
				seconds curTime = duration_cast<seconds>(system_clock::now().time_since_epoch());
				if (abs(duration_cast<seconds>(curTime - start).count()) >= (int)Constants::MaximumFinishRetryTimeValue)
				{
					cerr << "Unable to properly finish RDTP connection." << endl;
					break;
				}

				_printer.PrintInformation(ApplicationType::Client, packet3, retransmit, false);
				sendto(_sockfd, packet3.GetRawData().data(), packet3.GetRawDataSize(), 0, (struct sockaddr*)&_cli_addr, _cli_len);

				len = recvfrom(_sockfd, buf, Constants::MaxPacketSize, 0, (struct sockaddr*)&_cli_addr, &_cli_len);
				if (len > 0)
				{
					Packet packet = Packet::FromRawData(buf, len);
					_printer.PrintInformation(ApplicationType::Client, packet, false, true);

					if (packet.GetPacketType() == PacketType::FIN) // Skip sending ACK, just send FIN
					{
						retransmit = true;
						continue;
					}
					else if (packet.GetPacketType() != PacketType::ACK)
					{
						cerr << "Received unexpected packet, expected ACK." << endl;
						continue;
					}
					else
						break;
				}
				else
				{
					retransmit = true;
					continue;
				}
			}
		}

		_sendBase += 1;
		return;
	perror_then_failure:
		_Error("RDTP connection close failed");
	failure:
		return;
	}

	// Server
	void RDTPConnection::SendFinish()
	{
		ssize_t len;
		char buf[Constants::MaxPacketSize];

		bool retransmit = false;
		Packet packet = Packet(PacketType::FIN, _nextSeqNum, Constants::WindowSize, nullptr, 0);
		_nextSeqNum += 1;
		bool receiveFIN = true; // Still need to receive a FIN packet
		bool finReceived = false; // Was a FIN packet received

		if (!_setTimeout(_sockfd, 0, Constants::RetransmissionTimeoutValue_us))
			goto perror_then_failure;

		while (true)
		{
			_printer.PrintInformation(ApplicationType::Server, packet, retransmit, false);
			retransmit = true;
			sendto(_sockfd, packet.GetRawData().data(), packet.GetRawDataSize(), 0, (struct sockaddr*)&_cli_addr, _cli_len);

			// FIN-WAIT-1
			len = recv(_sockfd, buf, Constants::MaxPacketSize, 0);
			if (len <= 0) // Response was not received in 500 ms, retry
				continue;

			Packet packet = Packet::FromRawData(buf, len);
			_printer.PrintInformation(ApplicationType::Server, packet, false, true);

			if (packet.GetPacketType() == PacketType::ACK)
			{
				if (packet.GetNumber() != _sendBase)
					continue;
				else
					break;
			}
			else if (packet.GetPacketType() == PacketType::FIN)
			{
				_rcvBase = packet.GetNumber();
				receiveFIN = false;
				finReceived = true;
				break;
			}
			else
			{
				cerr << "Received unexpected packet, expected ACK or FIN." << endl;
				continue;
			}
		}

		// FIN-WAIT-2
		{
			_sendBase += 1;
			seconds start = duration_cast<seconds>(system_clock::now().time_since_epoch());
			if (!receiveFIN)
				goto send_ack;
			while (true)
			{
				{
					seconds curTime = duration_cast<seconds>(system_clock::now().time_since_epoch());
					auto elapsed = curTime - start;
					if (duration_cast<seconds>(elapsed).count() >= (int)Constants::MaximumFinishRetryTimeValue)
					{
						cout << "Timed wait finished." << endl;
						break;
					}

					auto remaining = seconds(Constants::MaximumFinishRetryTimeValue) - elapsed;
					auto remaining_usec = remaining - duration_cast<seconds>(remaining);
					_setTimeout(_sockfd, duration_cast<seconds>(remaining).count(), duration_cast<microseconds>(remaining_usec).count());
					len = recvfrom(_sockfd, buf, Constants::MaxPacketSize, 0, (struct sockaddr*)&_cli_addr, &_cli_len);
					if (len <= 0)
					{
						if (!finReceived)
							cout << "Did not receive FIN during timed wait." << endl;
						else
							cout << "Timed wait finished." << endl;
						break;
					}

					Packet packet = Packet::FromRawData(buf, len);
					_printer.PrintInformation(ApplicationType::Server, packet, false, true);

					if (packet.GetPacketType() != PacketType::FIN)
					{
						cerr << "Received unexpected packet, expected FIN." << endl;
						continue;
					}
					else
					{
						_rcvBase = packet.GetNumber();
						finReceived = true;
					}
				}

			send_ack:
				{
					Packet packet2 = Packet(PacketType::ACK, _rcvBase, Constants::WindowSize, nullptr, 0);
					_printer.PrintInformation(ApplicationType::Server, packet2, false, false);
					sendto(_sockfd, packet2.GetRawData().data(), packet2.GetRawDataSize(), 0, (struct sockaddr*)&_cli_addr, _cli_len);
				}
			}
		}
		_rcvBase += 1;
		return;
	perror_then_failure:
		_Error("RDTP connection close failed");
		return;
	}
}