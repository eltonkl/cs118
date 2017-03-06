#include "RDTP.h"
#include "Packet.h"

namespace RDTP
{
    Packet::Packet(unsigned char* data, size_t len) : _data(data, data + len)
    {
        ParseData();
    }

    Packet::Packet(PacketType type, int seq, int wnd, bool retransmission) :
        _packetType(type), _seq(seq), _wnd(wnd), _retransmission(retransmission)
    {
        GenerateData();
    }

    PacketType Packet::GetPacketType() const
    {
        return _packetType;
    }

    int Packet::GetSequenceNumber() const
    {
        return _seq;
    }

    int Packet::GetWindowSize() const
    {
        return _wnd;
    }

    bool Packet::GetIsRetransmission() const
    {
        return _retransmission;
    }

    void Packet::ParseData()
    {

    }

    void Packet::GenerateData()
    {

    }
}