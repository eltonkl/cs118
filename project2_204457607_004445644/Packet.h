#pragma once

#include <stdlib.h>
#include <vector>

namespace RDTP
{
    enum class PacketType
    {
        SYN,
        ACK,
        FIN
    };

    class Packet
    {
    public:
        Packet(unsigned char* data, size_t len);
        Packet(PacketType type, int seq, int wnd, bool retransmission);

        PacketType GetPacketType() const;
        int GetSequenceNumber() const;
        int GetWindowSize() const;
        bool GetIsRetransmission() const;

    private:
        void ParseData();
        void GenerateData();

        std::vector<unsigned char> _data;
        PacketType _packetType;
        int _seq;
        int _wnd;
        int _retransmission;
    };
}