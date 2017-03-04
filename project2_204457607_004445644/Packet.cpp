#include "RDTP.h"
#include "Packet.h"

namespace RDTP
{
    Packet::Packet(unsigned char* data, size_t len) : m_data(data, data + len)
    {
        ParseData();
    }

    Packet::Packet(PacketType type, int seq, int wnd, bool retransmission) :
        m_packetType(type), m_seq(seq), m_wnd(wnd), m_retransmission(retransmission)
    {
        GenerateData();
    }

    PacketType Packet::GetPacketType() const
    {
        return m_packetType;
    }

    int Packet::GetSequenceNumber() const
    {
        return m_seq;
    }

    int Packet::GetWindowSize() const
    {
        return m_wnd;
    }

    bool Packet::GetIsRetransmission() const
    {
        return m_retransmission;
    }

    void Packet::ParseData()
    {

    }

    void Packet::GenerateData()
    {

    }
}