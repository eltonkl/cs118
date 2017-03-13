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

	// Bytes
	const uint32_t Constants::MaxPacketSize = 1024;
	const uint32_t Constants::MaxSequenceNumber = 30720;
	const uint32_t Constants::WindowSize = 5120;
	const uint32_t Constants::HeaderSize = 8;
	// Milliseconds
	const uint32_t Constants::RetransmissionTimeoutValue = 500;
	const uint32_t Constants::RetransmissionTimeoutValue_us = Constants::RetransmissionTimeoutValue * 1000;

	const uint32_t Constants::MaximumFinishRetryTimeValue = 5;

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
			ReceiveFinish();
		else //if (_type == ApplicationType::Client)
			SendFinish();
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

		ssize_t len;
		char buf[Constants::MaxPacketSize];

		if (_type == ApplicationType::Client) {
			if (!_setTimeout(_sockfd, 0, 0))
				goto perror_then_failure;
			
			len = recv(_sockfd, buf, Constants::MaxPacketSize, 0);
			if (len > 0) {
				// Client got packet from Server
				Packet packet = Packet::FromRawData(buf, len);
				_printer.PrintInformation(ApplicationType::Client, packet, false, true);

				// TODO: handle count
				vector<char> data = packet.GetData();
				copy(data.begin(), data.end(), osi);


				// send ACK back
				{
					// TODO: update recv_base?

					// TODO: what's the SEQ in SR's ACK packet?
					Packet packet2 = Packet(PacketType::ACK, 404, packet.GetAcknowledgeNumber(), Constants::WindowSize, nullptr, 0);
					_printer.PrintInformation(ApplicationType::Client, packet2, false, false);
			
					// hope for the best :)
					write(_sockfd, packet2.GetRawData().data(), packet2.GetRawDataSize());
				}


			} else {
				cerr << "recv in client failed." << endl;
				goto perror_then_failure;
			}

		} else {
			// _type == ApplicationType::Server
			// TODO: Server Read

		}

	perror_then_failure:
		_Error("RDTP Read failed");
    }

	// HERE BE DRAGONS
	// DON'T SCROLL PAST THIS PART OF THE FILE, OR YOU MIGHT LOSE YOUR SANITY

	// Really should have refactored code into helper functions, but code quality isn't part of the grade

	// For the sender of a packet, the ACK # indicates how many bytes it has received from the other
	// For the sender of a packet, the SEQ # indicates how many bytes it has transferred successfully
	// For the receiver of a packet, the ACK # indicates how many bytes it has transferred sucessfully to the other
	// For the receiver of a packet, the SEQ # indicates how many bytes it has received from the other

	// Server
	void RDTPConnection::ReceiveHandshake()
	{
		ssize_t len;
		char buf[Constants::MaxPacketSize];

		// LISTEN
		if (!_setTimeout(_sockfd, 0, 0))
			goto perror_then_failure;
		len = recvfrom(_sockfd, buf, Constants::MaxPacketSize, 0, (struct sockaddr*)&_cli_addr, &_cli_len);
		if (len > 0)
		{
			Packet packet = Packet::FromRawData(buf, len);
			_printer.PrintInformation(ApplicationType::Server, packet, false, true);
		
			if (packet.GetPacketType() == PacketType::SYN)
				_sendBase = packet.GetSequenceNumber();
			else
			{
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

			_nextSeqNum = 0;
			_sendBase += 1;
			Packet packet = Packet(PacketType::SYNACK, _nextSeqNum, _sendBase, Constants::WindowSize, nullptr, 0);
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
				_nextSeqNum = packet.GetAcknowledgeNumber();
				_sendBase = packet.GetSequenceNumber();
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

			Packet packet2 = Packet(PacketType::ACK, _nextSeqNum, _sendBase, Constants::WindowSize, nullptr, 0);
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

	// Server
	void RDTPConnection::ReceiveFinish()
	{
		ssize_t len;
		char buf[Constants::MaxPacketSize];

		if (!_setTimeout(_sockfd, 0, 0))
			goto perror_then_failure;
		len = recvfrom(_sockfd, buf, Constants::MaxPacketSize, 0, (struct sockaddr*)&_cli_addr, &_cli_len);
		if (len > 0)
		{
			Packet packet = Packet::FromRawData(buf, len);
			_printer.PrintInformation(ApplicationType::Server, packet, false, true);

			if (packet.GetPacketType() == PacketType::FIN)
				_sendBase = packet.GetSequenceNumber();
			else
			{
				cerr << "Expected FIN packet, got something else." << endl;
				goto failure;
			}
		}
		else
		{
			cerr << "recvfrom failed." << endl;
			goto perror_then_failure;
		}

		{
			_sendBase += 1;

			Packet packet2 = Packet(PacketType::ACK, _nextSeqNum, _sendBase, Constants::WindowSize, nullptr, 0);
			_printer.PrintInformation(ApplicationType::Server, packet2, false, false);

			sendto(_sockfd, packet2.GetRawData().data(), packet2.GetRawDataSize(), 0, (struct sockaddr*)&_cli_addr, _cli_len);
		}

		// CLOSE_WAIT + LAST_ACK
		{
			Packet packet3 = Packet(PacketType::FIN, _nextSeqNum, _sendBase, Constants::WindowSize, nullptr, 0);
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

				_printer.PrintInformation(ApplicationType::Server, packet3, retransmit, false);
				sendto(_sockfd, packet3.GetRawData().data(), packet3.GetRawDataSize(), 0, (struct sockaddr*)&_cli_addr, _cli_len);

				len = recvfrom(_sockfd, buf, Constants::MaxPacketSize, 0, (struct sockaddr*)&_cli_addr, &_cli_len);
				if (len > 0)
				{
					Packet packet = Packet::FromRawData(buf, len);
					_printer.PrintInformation(ApplicationType::Server, packet, false, true);

					if (packet.GetPacketType() != PacketType::ACK)
					{
						cerr << "Received unexpected packet, expected ACK." << endl;
						goto failure;
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

		return;
	perror_then_failure:
		_Error("RDTP connection close failed");
	failure:
		return;
	}

	// Client
	void RDTPConnection::SendFinish()
	{
		ssize_t len;
		char buf[Constants::MaxPacketSize];

		bool retransmit = false;
		Packet packet = Packet(PacketType::FIN, _nextSeqNum, _sendBase, Constants::WindowSize, nullptr, 0);
		bool receiveFIN = true; // Still need to receive a FIN packet
		bool finReceived = false; // Was a FIN packet received

		if (!_setTimeout(_sockfd, 0, Constants::RetransmissionTimeoutValue_us))
			goto perror_then_failure;

		while (true)
		{
			_printer.PrintInformation(ApplicationType::Client, packet, retransmit, false);
			write(_sockfd, packet.GetRawData().data(), packet.GetRawDataSize());

			// FIN-WAIT-1
			len = recv(_sockfd, buf, Constants::MaxPacketSize, 0);
			if (len <= 0) // Response was not received in 500 ms, retry
			{
				retransmit = true;
				continue;
			}

			Packet packet = Packet::FromRawData(buf, len);
			_printer.PrintInformation(ApplicationType::Client, packet, false, true);

			_nextSeqNum = packet.GetAcknowledgeNumber(); // ?
			_sendBase = packet.GetSequenceNumber(); // ?

			if (packet.GetPacketType() == PacketType::ACK)
				break;
			else if (packet.GetPacketType() == PacketType::FIN)
			{
				receiveFIN = false;
				finReceived = true;
				break;
			}
			else
			{
				cerr << "Received unexpected packet, expected ACK or FIN." << endl;
				goto failure;
			}
		}

		// FIN-WAIT-2
		{
			seconds start = duration_cast<seconds>(system_clock::now().time_since_epoch());
			Packet packet2 = Packet(PacketType::ACK, _nextSeqNum, _sendBase, Constants::WindowSize, nullptr, 0);
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
					len = recv(_sockfd, buf, Constants::MaxPacketSize, 0);
					if (len <= 0)
					{
						if (!finReceived)
							cout << "Did not receive FIN during timed wait." << endl;
						else
							cout << "Timed wait finished." << endl;
						break;
					}

					Packet packet = Packet::FromRawData(buf, len);
					_printer.PrintInformation(ApplicationType::Client, packet, false, true);

					if (packet.GetPacketType() != PacketType::FIN)
					{
						cerr << "Received unexpected packet, expected FIN." << endl;
						goto failure;
					}
					else
						finReceived = true;
				}

			send_ack:
				_printer.PrintInformation(ApplicationType::Client, packet2, false, false);
				write(_sockfd, packet2.GetRawData().data(), packet2.GetRawDataSize());
			}
		}

		return;
	perror_then_failure:
		_Error("RDTP connection close failed");
	failure:
		return;
	}
}