#pragma once

#include <iostream>

#include "RDTP.h"
#include "Packet.h"

namespace RDTP
{
    enum class ApplicationType;

    namespace _Internals
    {
        // Has unncessary object oriented design gone too far? I think so
        class Printer
        {
        public:
            Printer(std::ostream& os);
            void PrintInformation(RDTP::ApplicationType type, const Packet& packet, bool retransmission);

        private:
            std::ostream& _os;

            void PrintInformationAsServer(const Packet& packet, bool retransmission);
            void PrintInformationAsClient(const Packet& packet, bool retransmission);
        };
    }
}