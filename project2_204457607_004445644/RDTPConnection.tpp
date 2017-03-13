namespace RDTP
{
    template <typename Iterator>
    std::vector<char> RDTPConnection::GetDataForNextPacket(Iterator& begin, const Iterator& end) {

        size_t maxDataSize = Constants::MaxPacketSize - Constants::HeaderSize;
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

        // if (std::distance(begin, end) >= maxDataSize) {
        //     std::vector<char> data(maxDataSize);
        //     // std::advance(begin, maxDataSize);
        //     std::vector<char>::iterator it = data.begin();
        //     while (it != data.end()) {
        //         *it++ = *begin++;
        //     }

        //     return data;
        // } else {
        //     std::vector<char> data(begin, end);
        //     begin = end;
        //     return data;
        // }

    }
    
    template <typename Iterator>
    void RDTPConnection::Write(Iterator begin, Iterator end)
    {
        // (void)begin;
        // (void)end;
        std::vector<char> data = GetDataForNextPacket(begin, end);
        std::vector<char>::iterator it = data.begin();
        std::cerr << "Data is: " << std::endl;
        while (it != data.end()) {
            std::cerr << *it++;
        }
        std::cerr << std::endl;

        // (std::vector<char>::iterator) begin;
        // (std::vector<char>::iterator) end;

        // std::cerr << "Data is: " << std::endl;
        // // std::vector<char>::iterator it = begin;
        // // while (it != end) {
        // //     std::cerr << *it;
        // // }
        // // std::cerr << std::endl;

        // // DO i put End?
        // // std::vector<char> data(std::istream_iterator<char>(begin), std::istream_iterator<char>(end));
        // // std::istream_iterator<char> it = std::istream_iterator<char>(begin);
        // // // char c =    it;
        // // while (true) {
        // //     std::cerr << *it;
        // // }
        // // std::cerr << std::endl;
        // // std::copy(std::istream_iterator<char>(begin), std::istream_iterator<char>(), std::ostream_iterator<char>(std::cerr));

        // // std::istream_iterator<char>(begin)
        // while (begin != end) {
        //     std::cerr << *begin++ << std::endl;
        // }

        // std::cerr << *begin << std::endl;

        // TODO: only one packet rn

        // TODO: check _nextSeqNum and _sendBase

        ssize_t len;
		char buf[Constants::MaxPacketSize];


        // if (!_setTimeout(_sockfd, 0, Constants::RetransmissionTimeoutValue_us))
		// 	goto perror_then_failure;



        if (_type == ApplicationType::Server) {

            // std::vector<char> data = GetDataForNextPacket(begin, end);
            std::vector<char> data = { 'h', 'e', 'l', 'l', 'o'};


            _Internals::Packet packet = _Internals::Packet(_Internals::PacketType::NONE, _nextSeqNum, _sendBase, Constants::WindowSize, data.data(), data.size());
			bool retransmit = false;

            _printer.PrintInformation(ApplicationType::Server, packet, retransmit, false);
			sendto(_sockfd, packet.GetRawData().data(), packet.GetRawDataSize(), 0, (struct sockaddr*)&_cli_addr, _cli_len);

            len = recvfrom(_sockfd, buf, Constants::MaxPacketSize, 0, (struct sockaddr*)&_cli_addr, &_cli_len);
            if (len > 0) {
                _Internals::Packet packet = _Internals::Packet::FromRawData(buf, len);
                _printer.PrintInformation(ApplicationType::Server, packet, false, true);
            
                // TODO check ACK number as well
                if (packet.GetPacketType() == _Internals::PacketType::ACK) {
                    // _sendBase = packet.GetSequenceNumber();
                    std::cerr << "Got ACK!!" << std::endl;
                } else {
                    std::cerr << "Expected ACK packet, got something else." << std::endl;
                    // goto failure;
                }
            } else {
                std::cerr << "recvfrom failed." << std::endl;
                goto perror_then_failure;
            }
        } else {
            // _type == ApplicationType::Client
        }

        perror_then_failure:
            std::cerr << "RIP" << std::endl;
		// _Error("RDTP handshake failed");

    }
}