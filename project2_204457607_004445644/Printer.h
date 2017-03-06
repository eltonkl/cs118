#pragma once

#include <iostream>
#include "Packet.h"

namespace RDTP
{
    class Printer
    {
    public:
        Printer(std::ostream& os);
        void Print(const Packet& packet);

    private:
        std::ostream& _os;
    };
}