#include "RDTP.h"
#include "Printer.h"

#include <iostream>
#include <string>

using namespace std;
using namespace RDTP;

namespace RDTP
{
    namespace _Internals
    {
        Printer::Printer(ostream& os) : _os(os) {}

        void Printer::PrintInformation(ApplicationType type, const Packet& packet, bool retransmission, bool isReceive)
        {
            if (type == ApplicationType::Server)
                PrintInformationAsServer(packet, retransmission, isReceive);
            else
                PrintInformationAsClient(packet, retransmission, isReceive);
        }

        void Printer::PrintInformationAsServer(const Packet& packet, bool retransmission, bool isReceive)
        {
            PacketType pt = packet.GetPacketType();

            if (isReceive)
                _os << "Receiving packet ";
            else
                _os << "Sending packet ";

            switch (pt)
            {
            case PacketType::SYN:
                _os << packet.GetSequenceNumber();
                _os << " " << packet.GetWindowSize();
                if (retransmission)
                    _os << " Retransmission";
                _os << " SYN" << endl;
                break;
            case PacketType::ACK:
                _os << packet.GetAcknowledgeNumber() << endl;
                break;
            case PacketType::FIN:
                _os << packet.GetSequenceNumber();
                _os << " " << packet.GetWindowSize();
                if (retransmission)
                    _os << " Retransmission";
                _os << " FIN" << endl;
                break;
            case PacketType::NONE:
                _os << packet.GetSequenceNumber();
                _os << " " << packet.GetWindowSize();
                if (retransmission)
                    _os << " Retransmission";
                _os << endl;
                break;
            case PacketType::SYNACK:
                _os << packet.GetSequenceNumber();
                _os << " " << packet.GetWindowSize();
                if (retransmission)
                    _os << " Retransmission";
                _os << " SYNACK" << endl;
            }
        }

        void Printer::PrintInformationAsClient(const Packet& packet, bool retransmission, bool isReceive)
        {
            PacketType pt = packet.GetPacketType();

            if (isReceive)
                _os << "Receiving packet ";
            else
                _os << "Sending packet ";

            switch (pt)
            {
            case PacketType::SYN:
                _os << packet.GetSequenceNumber();
                if (retransmission)
                    _os << " Retransmission";
                _os << " SYN" << endl;
                break;
            case PacketType::ACK:
                _os << packet.GetAcknowledgeNumber() << endl;
                if (retransmission)
                    _os << " Retransmission";
                _os << " FIN" << endl;
                break;
            case PacketType::FIN:
                _os << packet.GetAcknowledgeNumber();
                if (retransmission)
                    _os << " Retransmission";
                _os << " FIN" << endl;
                break;
            case PacketType::NONE:
                _os << packet.GetSequenceNumber() << endl;
                break;
            case PacketType::SYNACK:
                _os << packet.GetSequenceNumber() << endl;
            }
        }
    }
}