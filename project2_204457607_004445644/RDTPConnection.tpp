namespace RDTP
{
    template <typename Iterator>
    std::vector<char> RDTPConnection::GetDataForNextPacket(Iterator& begin, const Iterator& end, const size_t maxSize) {

        // maxDataSize is either from the passed in param or Constants
        size_t maxDataSize = std::min<size_t>(maxSize, Constants::MaxPacketSize - Constants::HeaderSize);
        size_t vSize;
        if (std::distance(begin, end) >= (int)maxDataSize) {
            vSize = maxDataSize;
        } else {
            vSize = std::distance(begin, end);
        }

        std::vector<char> data(vSize);
        std::vector<char>::iterator it = data.begin();

        while (vSize--) {
            *it++ = *begin++;
        }

        return data;
    }

    namespace _Internals
    {
        class Compare
        {
        public:
            bool operator() (uint16_t a, uint16_t b) {
                return a > b;
            }
        };
    }
    
    template <typename Iterator>
    void RDTPConnection::Write(Iterator begin, Iterator end)
    {
        using namespace std;
        using namespace std::chrono;
        using namespace RDTP;
        using namespace RDTP::_Internals;

        ssize_t len;
		char buf[Constants::MaxPacketSize];

        unordered_map<uint16_t, size_t> packetSizes;    // _seq -> packet size
        list<pair<Packet, milliseconds>> timestamps;    // list of Packet and their respective timestamps
        unordered_map<uint16_t, list<pair<Packet, milliseconds>>::iterator> packetTimestamps; // _seq -> above
        priority_queue<uint16_t, vector<uint16_t>, _Internals::Compare> minACK;


        while (true) {
            // *****************************
            // Step 1
            // for all packets that timed out
            //     resend packet
            //     reset timestamp
            milliseconds curTime;
            for (list<pair<Packet, milliseconds>>::iterator it = timestamps.begin(); it != timestamps.end(); ++it) {
                curTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
                if (abs(duration_cast<milliseconds>(curTime - it->second).count()) >= (int)Constants::RetransmissionTimeoutValue) {
                    // this Packet timeout
                    _printer.PrintInformation(_type, it->first, true, false);

                    it->second = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
                    if (_type == ApplicationType::Client) {
                        write(_sockfd, it->first.GetRawData().data(), it->first.GetRawDataSize());
                    } else {
                        // _type == ApplicationType::Server
                        sendto(_sockfd, it->first.GetRawData().data(), it->first.GetRawDataSize(), 0, (struct sockaddr*)&_cli_addr, _cli_len);
                    }
                }
            }

            // *****************************
            // Step 2
            // if nextseqnum < send_base + window_size
            //     prepare new packet (size == min(max packet size, send_base + window_size - nextseqnum))
            //     send packet
            //     update nextseqnum += size
            if (_nextSeqNum < _sendBase + Constants::WindowSize) {
                vector<char> data = GetDataForNextPacket(begin, end, (size_t) (_sendBase + Constants::WindowSize - _nextSeqNum));

                if (data.size() == 0 && packetSizes.size() == 0) {
                    // no more stuff to send
                    // no waiting on any other ACK
                    break;
                }

                uint16_t realPacketNum = _sendBase % Constants::MaxSequenceNumber;
                Packet packet = Packet(PacketType::NONE, realPacketNum, Constants::WindowSize, data.data(), data.size());
                
                // update data structures
                packetSizes[realPacketNum] = data.size();
                timestamps.emplace_back(packet, duration_cast<milliseconds>(system_clock::now().time_since_epoch()));
                list<pair<Packet, milliseconds>>::iterator temp = timestamps.end();
                packetTimestamps[realPacketNum] = --temp;

                // send packet
                _printer.PrintInformation(_type, packet, true, false);
                if (_type == ApplicationType::Client) {
                    write(_sockfd, packet.GetRawData().data(), packet.GetRawDataSize());
                } else {
                    // _type == ApplicationType::Server
                    sendto(_sockfd, packet.GetRawData().data(), packet.GetRawDataSize(), 0, (struct sockaddr*)&_cli_addr, _cli_len);
                }

                _nextSeqNum += data.size();
            }

            // *****************************
            // Step 3
            // try to receive packet for RTO time (500ms)
            //     if send_base <= ACK <= send_base + window_size
            //         add to minheap
            //         delete packet(ACK#) from list
            if (!_setTimeout(_sockfd, 0, Constants::RetransmissionTimeoutValue_us))
			    goto perror_then_failure;
            while (true) {
                len = recvfrom(_sockfd, buf, Constants::MaxPacketSize, 0, (struct sockaddr*)&_cli_addr, &_cli_len);

                if (len > 0) {
                    Packet packet = Packet::FromRawData(buf, len);

                    uint16_t realSendBase = _sendBase % Constants::MaxSequenceNumber;
                    if (packet.GetPacketType() == PacketType::ACK) {
                        // check validity of ACK #
                        bool rotated = false;
                        if (realSendBase + Constants::WindowSize > Constants::MaxSequenceNumber) {
                            rotated = true;
                        }

                        if ((!rotated && realSendBase <= packet.GetNumber() && packet.GetNumber() <= realSendBase + Constants::WindowSize) 
                            || (rotated && packet.GetNumber() <= (realSendBase + Constants::WindowSize) % Constants::MaxSequenceNumber))
                        {
                            // send_base <= ACK <= send_base + window_size
                            uint16_t ackNum = packet.GetNumber();
                            minACK.push(ackNum);
                            timestamps.erase(packetTimestamps[ackNum]);
                            packetTimestamps.erase(ackNum);
                        }
                    }
                } else {
                    // timeout
                    break;
                }

                // *****************************
                // Step 4
                // while minheap.top() == send_base
                //    send_base = send_base + size of packet(minheap.top())
                //    delete packet with minheap.top() == ACK from table
                //    update smallestReceivedACK from minheap 
                while (_sendBase % Constants::MaxSequenceNumber == minACK.top()) {
                    _sendBase += packetSizes[minACK.top()];
                    packetSizes.erase(minACK.top());
                    minACK.pop();
                }

            }
        }
        perror_then_failure:
		    _Internals::_Error("_setTimeout failed rip");
    }
}