#include <vector>
#include <cassert>

#include "RDTP.h"
#include "Packet.h"

using namespace std;
using namespace RDTP;

// This code isn't portable

/*
    Packet header layout:
    Each row is 32-bits, 64-bits total.
    SEQ (16-bit int), ACK (16-bit int)
    [SYN, ACK, FIN] flags (1-bit each), 00000000000 (11-bits), WND (16-bit int)
*/

namespace RDTP
{
    namespace _Internals
    {
        // Initialize class from raw packet data
        Packet Packet::FromRawData(char* rawData, size_t rawDataLength)
        {
            return Packet(rawData, rawDataLength);
        }

        // Construct packet using given parameters and data
        Packet::Packet(PacketType type, uint16_t seq, uint16_t ack, uint16_t wnd, char* data, size_t dataLength) :
            _packetType(type), _seq(seq), _ack(ack), _wnd(wnd), _valid(true)
        {
            assert(dataLength <= Constants::MaxPacketSize - Constants::HeaderSize);
            _rawData.reserve(Constants::HeaderSize + dataLength);
            GenerateHeader();
            if (data)
                _rawData.insert(_rawData.begin() + Constants::HeaderSize, data, data + dataLength);
        }

        PacketType Packet::GetPacketType() const
        {
            return _packetType;
        }

        uint16_t Packet::GetSequenceNumber() const
        {
            return _seq;
        }

        uint16_t Packet::GetAcknowledgeNumber() const
        {
            return _ack;
        }

        uint16_t Packet::GetWindowSize() const
        {
            return _wnd;
        }

        bool Packet::GetValid() const
        {
            return _valid;
        }

        vector<char> Packet::GetData() const
        {
            return vector<char>(_rawData.begin() + Constants::HeaderSize, _rawData.end());
        }

        const vector<char>& Packet::GetRawData() const
        {
            return _rawData;
        }

        size_t Packet::GetDataSize() const
        {
            return _rawData.size() - Constants::HeaderSize;
        }

        size_t Packet::GetRawDataSize() const
        {
            return _rawData.size();
        }

        Packet::Packet(char* rawData, size_t rawDataLength) :
            _rawData(rawData, rawData + rawDataLength), _valid(true)
        {
            ParseRawData();
        }

        void Packet::ParseRawData()
        {
            // Should have just used C-style casts
            _seq = (static_cast<uint16_t>(static_cast<unsigned char>(_rawData[0])) << 16) | static_cast<uint16_t>(static_cast<unsigned char>(_rawData[1]));
            _ack = (static_cast<uint16_t>(static_cast<unsigned char>(_rawData[2])) << 16) | static_cast<uint16_t>(static_cast<unsigned char>(_rawData[3]));
            switch (static_cast<unsigned char>(_rawData[4]))
            {
            case 0b10000000:
                _packetType = PacketType::SYN;
                break;
            case 0b01000000:
                _packetType = PacketType::ACK;
                break;
            case 0b00100000:
                _packetType = PacketType::FIN;
                break;
            case 0b00000000:
                _packetType = PacketType::NONE;
            case 0b11000000:
                _packetType = PacketType::SYNACK;
            default:
                _valid = false;
                break;
            }
            _wnd = (static_cast<uint16_t>(static_cast<unsigned char>(_rawData[6])) << 16) | static_cast<uint16_t>(static_cast<unsigned char>(_rawData[7]));
        }

        void Packet::GenerateHeader()
        {
            char byte;
            byte = static_cast<char>(_seq >> 8);
            _rawData.push_back(byte);
            byte = static_cast<char>(_seq);
            _rawData.push_back(byte);
            byte = static_cast<char>(_ack >> 8);
            _rawData.push_back(byte);
            byte = static_cast<char>(_ack);
            _rawData.push_back(byte);
            switch (_packetType)
            {
            case PacketType::SYN:
                byte = 0b10000000;
                break;
            case PacketType::ACK:
                byte = 0b01000000;
                break;
            case PacketType::FIN:
                byte = 0b00100000;
                break;
            case PacketType::NONE:
                byte = 0b00000000;
            case PacketType::SYNACK:
                byte = 0b11000000;
            }
            _rawData.push_back(byte);
            _rawData.push_back(0);
            byte = static_cast<char>(_wnd >> 8);
            _rawData.push_back(byte);
            byte = static_cast<char>(_wnd);
            _rawData.push_back(byte);
        }
    }
}