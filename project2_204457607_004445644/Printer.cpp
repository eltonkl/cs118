#include "RDTP.h"
#include "Printer.h"

#include <iostream>
#include <string>

using namespace std;

namespace RDTP
{
    Printer::Printer(ostream& os) : m_os(os) {}

    void Printer::Print(const Packet& packet)
    {
        PacketType pt = packet.GetPacketType();
        (void) pt;
    }
}