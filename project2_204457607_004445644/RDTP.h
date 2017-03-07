#pragma once

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
}