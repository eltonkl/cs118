#define DEBUG

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

#ifdef DEBUG
        void Printer::PrintInformation(ApplicationType type, const Packet& packet, bool retransmission, bool isReceive)
        {
            if (type == ApplicationType::Server)
                _os << "[Server] ";
            else 
                _os << "[Client] ";

            if (isReceive)
                _os << "Receiving packet: ";
            else
                _os << "Sending packet: ";

            PacketType pt = packet.GetPacketType();
            _os << "Type: ";
            switch (pt) {
                case PacketType::SYN: _os << "SYN; "; break;
                case PacketType::ACK: _os << "ACK; "; break;
                case PacketType::FIN: _os << "FIN; "; break;
                case PacketType::NONE: _os << "NONE; "; break;
                case PacketType::SYNACK: _os << "SYNACK; "; break;
            }

            _os << "SEQ/ACK: " << packet.GetNumber() << "; ";
            // TODO: fix window size
            _os << "WND: " << packet.GetWindowSize() << "; ";

            if (retransmission)
                _os << " Retransmission";
            _os << endl;
        }
#else
        void Printer::PrintInformation(ApplicationType type, const Packet& packet, bool retransmission, bool isReceive)
        {
            PacketType pt = packet.GetPacketType();
            if (isReceive) {
                if (type == ApplicationType::Server) {
                    // “Receiving packet” [ACK number]
                    _os << "Receiving packet " << packet.GetAcknowledgeNumber() << endl;
                } else {
                    // Client
                    // “Receiving packet” [Sequence number]
                    _os << "Receiving packet " << packet.GetSequenceNumber() << endl;
                }
            } else {
                if (type == ApplicationType::Server) {
                    // “Sending packet” [Sequence number] [WND] (“Retransmission”) (“SYN”) (“FIN”)
                    _os << "Sending packet " << packet.GetSequenceNumber();
                    _os << " " << packet.GetWindowSize();
                } else {
                    // Client
                    // “Sending packet” [ACK number] (“Retransmission”) (”SYN”) (”FIN”)
                    _os << "Sending packet";
                    if (pt != PacketType::SYN)
                        _os << " " << packet.GetAcknowledgeNumber();
                }

                // common between Cleint/Server sending
                if (retransmission)
                    _os << " Retransmission";
                if (pt == PacketType::SYN)
                    _os << " SYN";
                if (pt == PacketType::FIN)
                    _os << " FIN";
                _os << endl;
            }
        }
#endif
    }
}