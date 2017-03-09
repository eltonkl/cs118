#pragma once

#include <iostream>
#include "Packet.h"

namespace RDTP
{
    namespace _Internals
    {
        enum class ApplicationType
        {
            Server,
            Client
        };

        // Has unncessary object oriented design gone too far? I think so
        class Printer
        {
        public:
            Printer(std::ostream& os);
            void PrintInformation(ApplicationType type, const Packet& packet, bool retransmission);

        private:
            std::ostream& _os;

            void PrintInformationAsServer(const Packet& packet, bool retransmission);
            void PrintInformationAsClient(const Packet& packet, bool retransmission);
        };
    }
}