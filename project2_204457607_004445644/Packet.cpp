#include <vector>
#include <cassert>

#include "RDTP.h"
#include "Packet.h"

using namespace std;
using namespace RDTP;

// This code isn't portable

/*
    Packet header layout:
    Packet is 5 bytes in size.
    NUM (16-bit int), [SYN, ACK, FIN] flags (1-bit each), 00000 (5-bits), WND (16-bit int)
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
        Packet::Packet(PacketType type, uint16_t num, uint16_t wnd, char* data, size_t dataLength) :
            _packetType(type), _num(num), _wnd(wnd), _valid(true)
        {
            assert(dataLength <= Constants::MaxPacketSize - Constants::HeaderSize);
            _rawData.reserve(Constants::HeaderSize + dataLength);
            GenerateHeader();
            if (data)
                _rawData.insert(_rawData.begin() + Constants::HeaderSize, data, data + dataLength);
        }

        Packet::Packet() : _valid(false) {}

        PacketType Packet::GetPacketType() const
        {
            return _packetType;
        }

        uint16_t Packet::GetNumber() const
        {
            return _num;
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
            _num = (static_cast<uint16_t>(static_cast<unsigned char>(_rawData[0])) << 8) | static_cast<uint16_t>(static_cast<unsigned char>(_rawData[1]));
            switch (static_cast<unsigned char>(_rawData[2]))
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
                break;
            case 0b11000000:
                _packetType = PacketType::SYNACK;
                break;
            default:
                _valid = false;
                break;
            }
            _wnd = (static_cast<uint16_t>(static_cast<unsigned char>(_rawData[3])) << 8) | static_cast<uint16_t>(static_cast<unsigned char>(_rawData[4]));
        }

        void Packet::GenerateHeader()
        {
            char byte;
            byte = static_cast<char>(_num >> 8);
            _rawData.push_back(byte);
            byte = static_cast<char>(_num);
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
                break;
            case PacketType::SYNACK:
                byte = 0b11000000;
                break;
            }
            _rawData.push_back(byte);
            byte = static_cast<char>(_wnd >> 8);
            _rawData.push_back(byte);
            byte = static_cast<char>(_wnd);
            _rawData.push_back(byte);
        }
    }
}