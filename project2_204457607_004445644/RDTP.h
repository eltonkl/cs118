#pragma once

#include <vector>
#include <istream>
#include <ostream>
#include <utility>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "Packet.h"
#include "Printer.h"

namespace RDTP
{
    struct Constants
    {
        // Bytes
		const static size_t MaxPacketSize; // = 1024;
		const static size_t MaxSequenceNumber; // = 30720;
		const static size_t WindowSize; // = 5120;
		const static size_t HeaderSize; // = 8;
        // Milliseconds
		const static size_t RetransmissionTimeoutValue; // = 500;
        const static size_t RetransmissionTimeoutValue_us; // = 500;

		const static size_t InitialSlowStartThreshold; // = 15360;
		const static size_t InitialCongestionWindowSize; // = 1024;
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
        struct sockaddr_in _cli_addr;
        uint16_t _nextSeqNum;
        uint16_t _sendBase;
        bool _established;

        void ReceiveHandshake();
        void InitiateHandshake();

        void ReceiveFinish();
        void SendFinish();
    };
}

#include "RDTPConnection.tpp"