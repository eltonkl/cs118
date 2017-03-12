#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <vector>

/*
    Packet header layout:
    Each row is 32-bits, 64-bits total.
    SEQ (16-bit int), ACK (16-bit int)
    [SYN, ACK, FIN] flags (1-bit each), 00000 (5-bits), WND (16-bit int)
*/

namespace RDTP
{
    namespace _Internals
    {
        enum class PacketType
        {
            SYN,
            ACK,
            FIN,
            NONE,
            SYNACK
        };

        class Packet
        {
        public:
            static Packet FromRawData(char* rawData, size_t rawDataLength);
            Packet(PacketType type, uint16_t seq, uint16_t ack, uint16_t wnd, char* data, size_t dataLength);

            PacketType GetPacketType() const;
            uint16_t GetSequenceNumber() const;
            uint16_t GetAcknowledgeNumber() const;
            uint16_t GetWindowSize() const;
            bool GetValid() const;
            std::vector<char> GetData() const;
            const std::vector<char>& GetRawData() const;
            size_t GetDataSize() const;
            size_t GetRawDataSize() const;

        private:
            Packet(char* rawData, size_t dataLength);

            void ParseRawData();
            void GenerateHeader();

            std::vector<char> _rawData;
            PacketType _packetType;
            uint16_t _seq;
            uint16_t _ack;
            uint16_t _wnd;
            bool _valid;
        };
    }
}