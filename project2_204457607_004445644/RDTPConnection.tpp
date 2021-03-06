namespace RDTP
{
    template <typename Iterator>
    std::vector<char> RDTPConnection::GetDataForNextPacket(Iterator& begin, const Iterator& end, const size_t maxSize) {
        size_t len = 0;
        size_t maxDataSize = std::min<size_t>(maxSize, Constants::MaxPacketSize - Constants::HeaderSize);
        std::vector<char> data;

        while (begin != end && len++ < maxDataSize)
            data.push_back(*begin++);

        return data;
    }

    namespace _Internals
    {
        class Compare
        {
        public:
            bool operator() (uint64_t a, uint64_t b) {
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

        unordered_map<uint64_t, size_t> packetSizes;    // _seq -> packet size
        list<pair<Packet, milliseconds>> timestamps;    // list of Packet and their respective timestamps
        unordered_map<uint64_t, list<pair<Packet, milliseconds>>::iterator> packetTimestamps; // _seq -> above
        priority_queue<uint64_t, vector<uint64_t>, _Internals::Compare> minACK;
        if (!_setTimeout(_sockfd, 0, Constants::RetransmissionTimeoutValue_us))
            goto perror_then_failure;

        while (true) {
            // *****************************
            // Step 1
            // for the earliest packet that timed out
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
            // while nextseqnum < send_base + window_size
            //     prepare new packet (size == min(max packet size, send_base + window_size - nextseqnum))
            //     send packet
            //     update nextseqnum += size
            bool breakOut = false;
            //cout << "nextseqnum: " << _nextSeqNum << endl;
            while (_nextSeqNum < _sendBase + Constants::WindowSize) {
                vector<char> data = GetDataForNextPacket(begin, end, (size_t) (_sendBase + Constants::WindowSize - _nextSeqNum));

                if (data.size() == 0) {
                    // no more stuff to send
                    breakOut = true;
                    break;
                }

                if (data.size() != 0) {
                    uint16_t realPacketNum = _nextSeqNum % Constants::MaxSequenceNumber;
                    Packet packet = Packet(PacketType::NONE, realPacketNum, Constants::WindowSize, data.data(), data.size());

                    // update data structures
                    packetSizes[_nextSeqNum] = data.size();
                    timestamps.emplace_back(packet, duration_cast<milliseconds>(system_clock::now().time_since_epoch()));
                    list<pair<Packet, milliseconds>>::iterator temp = timestamps.end();
                    packetTimestamps[_nextSeqNum] = --temp;

                    // send packet
                    _printer.PrintInformation(_type, packet, false, false);
                    if (_type == ApplicationType::Client) {
                        write(_sockfd, packet.GetRawData().data(), packet.GetRawDataSize());
                    } else {
                        // _type == ApplicationType::Server
                        sendto(_sockfd, packet.GetRawData().data(), packet.GetRawDataSize(), 0, (struct sockaddr*)&_cli_addr, _cli_len);
                    }

                    _nextSeqNum += data.size();
                }
            }

            if (breakOut && packetSizes.size() == 0) // no more stuff to send & no waiting on any other ACK
                break;

            // *****************************
            // Step 3
            // try to receive packet for RTO time (500ms)
            //     if send_base <= ACK <= send_base + window_size
            //         add to minheap
            //         delete packet(ACK#) from list
            //while (true) {
                len = recvfrom(_sockfd, buf, Constants::MaxPacketSize, 0, (struct sockaddr*)&_cli_addr, &_cli_len);

                if (len > 0) {
                    Packet packet = Packet::FromRawData(buf, len);

                    if (packet.GetPacketType() == PacketType::ACK) {
                        uint64_t actualAckNumber;
                        if ((_sendBase % Constants::MaxSequenceNumber) + Constants::WindowSize > Constants::MaxSequenceNumber)
                        {
                            if (packet.GetNumber() < (_sendBase % Constants::MaxSequenceNumber) && packet.GetNumber() <= Constants::WindowSize)
                                actualAckNumber = packet.GetNumber() + _sendBase + (Constants::MaxSequenceNumber - (_sendBase % Constants::MaxSequenceNumber));
                            else
                                actualAckNumber = packet.GetNumber() + _roundDown(_sendBase);
                        }
                        else
                            actualAckNumber = packet.GetNumber() + _roundDown(_sendBase);
                        // check validity of ACK #
                        //cout << actualAckNumber << " " << "<- actual ack || sendBase -> " << _sendBase << endl;
                        if (_sendBase <= actualAckNumber && actualAckNumber <= _sendBase + Constants::WindowSize)
                        {
                            // send_base <= ACK <= send_base + window_size
                            _printer.PrintInformation(_type, packet, false, true);
                            minACK.push(actualAckNumber);
                            timestamps.erase(packetTimestamps[actualAckNumber]);
                            packetTimestamps.erase(actualAckNumber);
                        }
                    }
                    else {
                        _printer.PrintInformation(_type, packet, false, true);
                        uint16_t ackNum = packet.GetNumber();
                        if (ackNum == _rcvBase % Constants::MaxSequenceNumber)
                        {
                            _firstDataPacket = new Packet(packet);
                            return;
                        }
                    }
                }
                // if (!minACK.empty())
                //     cout << "minACK: " << minACK.top() << " ";
                // cout << "_sendBase: " << _sendBase;
                // else {
                    // timeout
                //    break;
                //}
            //}

            // *****************************
            // Step 4
            // while minheap.top() == send_base
            //    send_base = send_base + size of packet(minheap.top())
            //    delete packet with minheap.top() == ACK from table
            //    update smallestReceivedACK from minheap 
            while (!minACK.empty() && _sendBase == minACK.top()) {
                // cout << " minACK.top(): " << minACK.top();
                _sendBase += packetSizes[minACK.top()];
                packetSizes.erase(minACK.top());
                minACK.pop();
            }
            // cout << endl;
        }
        return;
        perror_then_failure:
		    _Internals::_Error("_setTimeout failed rip");
    }
}