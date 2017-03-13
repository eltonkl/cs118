namespace RDTP
{
    template <typename Iterator>
    std::vector<char> RDTPConnection::GetDataForNextPacket(Iterator& begin, const Iterator& end) {

        size_t maxDataSize = Constants::MaxPacketSize - Constants::HeaderSize;
        if (std::distance(begin, end) >= maxDataSize) {
            std::vector<char> data(maxDataSize);
            // std::advance(begin, maxDataSize);
            std::vector<char>::iterator it = data.begin();
            while (it != data.end()) {
                *it++ = *begin++;
            }

            return data;
        } else {
            std::vector<char> data(begin, end);
            begin = end;
            return data;
        }

    }
    
    template <typename Iterator>
    void RDTPConnection::Write(Iterator begin, Iterator end)
    {
        // (void)begin;
        // (void)end;

        // TODO: only one packet rn

        // TODO: check _nextSeqNum and _sendBase

        ssize_t len;
		char buf[Constants::MaxPacketSize];


        // if (!_setTimeout(_sockfd, 0, Constants::RetransmissionTimeoutValue_us))
		// 	goto perror_then_failure;



        if (_type == ApplicationType::Server) {

            std::vector<char> data = GetDataForNextPacket(begin, end);
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











    //     bool retransmit = false;

    //     vector<char> data = GetDataForNextPacket(begin, end);

    //     Packet packet = Packet(PacketType::NONE, _nextSeqNum, _sendBase, Constants::WindowSize, data, data.size());

    //     if (!_setTimeout(_sockfd, 0, Constants::RetransmissionTimeoutValue_us)) {
	// 		goto perror_then_failure;
    //     }

    //     while (true)
	// 	{
	// 		_printer.PrintInformation(_type, packet, retransmit, false);
	// 		write(_sockfd, packet.GetRawData().data(), packet.GetRawDataSize());

	// 		// wait for ACK
    //         // assume no concurrent connection in server
	// 		len = recv(_sockfd, buf, Constants::MaxPacketSize, 0);
	// 		if (len == 0) // Response was not received in 500 ms, retry
	// 		{
	// 			retransmit = true;
	// 			continue;
	// 		}

    //         Packet packet = Packet::FromRawData(buf, len);
	// 		_printer.PrintInformation(ApplicationType::Client, packet, false, true);

	// 		if (packet.GetPacketType() == PacketType::ACK)
	// 		{
	// 			_nextSeqNum = packet.GetAcknowledgeNumber();
	// 			_sendBase = packet.GetSequenceNumber();
	// 			break;
	// 		}
	// 		else
	// 		{
	// 			cerr << "Received unexpected packet, expected SYNACK." << endl;
	// 			goto failure;
	// 		}

    //     }
    // perror_then_failure:
	// 	_Error("RDTP handshake failed");
	// failure:
	// 	_established = false;
    }
}