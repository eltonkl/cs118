#pragma once

#include <iostream>
#include "Packet.h"

namespace RDTP
{
    enum class ApplicationType
    {
        Server,
        Client
    };

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