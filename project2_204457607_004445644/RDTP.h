#pragma once

#include <vector>
#include <chrono>
#include <istream>
#include <ostream>
#include <utility>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unordered_map>
#include <list>
#include <queue>
#include <vector>
#include <algorithm>

#include "Packet.h"
#include "Printer.h"

namespace RDTP
{
    struct Constants
    {
        // Bytes
		const static uint32_t MaxPacketSize; // = 1024;
		const static uint32_t MaxSequenceNumber; // = 30720;
		const static uint32_t WindowSize; // = 5120;
		const static uint32_t HeaderSize; // = 8;
        // Milliseconds
		const static uint32_t RetransmissionTimeoutValue; // = 500;
        // Microseconds
        const static uint32_t RetransmissionTimeoutValue_us; // = 500000;
        // Seconds
        const static uint32_t MaximumFinishRetryTimeValue;

		const static uint32_t InitialSlowStartThreshold; // = 15360;
		const static uint32_t InitialCongestionWindowSize; // = 1024;
    };

    enum class ApplicationType
    {
        Server,
        Client
    };

    // Assume things don't fail
    class RDTPConnection
    {
    public:
        // Three way handshake
        RDTPConnection(ApplicationType type, const int sockfd);
        // Close connection protocol
        ~RDTPConnection();
        bool IsConnectionEstablished() const;

        // Write data
        template <typename Iterator>
        void Write(Iterator begin, Iterator end);
        RDTPConnection& operator<<(std::basic_istream<char>& is);
        // Read data
        void Read(std::basic_ostream<char>& os, size_t count);

    private:
        const int _sockfd;
        socklen_t _cli_len;
        _Internals::Printer _printer;
        ApplicationType _type;
        struct sockaddr_in _cli_addr;
        uint64_t _nextSeqNum;
        uint64_t _sendBase;
        uint64_t _rcvBase;
        bool _established;
        // If the three-way handshake on the server's
        // behalf received a data packet instead of an
        // ACK (aka the ACK dropped), this will not be
        // nullptr
        _Internals::Packet* _firstDataPacket;

        template <typename Iterator>
        std::vector<char> GetDataForNextPacket(Iterator& begin, const Iterator& end, const size_t maxSize);

        void ReceiveHandshake();
        void InitiateHandshake();

        void ReceiveFinish();
        void SendFinish();
    };
}

#include "RDTPConnection.tpp"