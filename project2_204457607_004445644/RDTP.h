#pragma once

#include <vector>
#include <netinet/in.h>
#include <sys/socket.h>

#include "Packet.h"
#include "Printer.h"

namespace RDTP
{
    struct Constants
    {
        // Bytes
        const static int MaxPacketSize = 1024;
        const static int MaxSequenceNumber = 30720;
        const static int WindowSize = 5120;
        const static int HeaderSize = 8;
        // Milliseconds
        const static int RetransmissionTimeoutValue = 500;

        const static int InitialSlowStartThreshold = 15360;
        const static int InitialCongestionWindowSize = 1024;
    };

    // We assume that things can never fail because this is a perfect world :)
    class RDTPIOStream
    {
    public:
        // Three way handshake
        RDTPIOStream(const int sockfd, const struct sockaddr_in& cli_addr);
        
        // Send data
        RDTPIOStream& operator<<(const std::vector<unsigned char>& data);
        // Receive data
        RDTPIOStream& operator>>(std::vector<unsigned char>& data);

    private:
        const int _sockfd;
        const struct sockaddr_in _cli_addr;
        const socklen_t _cli_len;
        _Internals::Printer _printer;
    };
}