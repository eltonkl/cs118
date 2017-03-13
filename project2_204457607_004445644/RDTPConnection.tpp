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
    }
    
    template <typename Iterator>
    void RDTPConnection::Write(Iterator begin, Iterator end)
    {
        // TODO: one only packet currently
        // TODO: check _nextSeqNum and _sendBase
        
        std::vector<char> data = GetDataForNextPacket(begin, end);

        ssize_t len;
		char buf[Constants::MaxPacketSize];


        // if (!_setTimeout(_sockfd, 0, Constants::RetransmissionTimeoutValue_us))
		// 	goto perror_then_failure;

        if (_type == ApplicationType::Server) {

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
                    // TODO:
                    // update send_base


                } else {
                    std::cerr << "Expected ACK packet, got something else." << std::endl;
                    // goto failure;
                }
            } else {
                // TODO: timeout?
                std::cerr << "recvfrom failed." << std::endl;
                goto perror_then_failure;
            }
        } else {
            // _type == ApplicationType::Client
            // TODO: Client write
        }

        perror_then_failure:
            std::cerr << "RIP" << std::endl;
		// _Error("RDTP handshake failed");

    }
}