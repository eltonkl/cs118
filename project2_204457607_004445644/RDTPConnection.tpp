namespace RDTP
{
    template <typename Iterator>
    std::vector<char> RDTPConnection::GetDataForNextPacket(Iterator& begin, const Iterator& end) {

        size_t maxDataSize = Constants::MaxPacketSize - Constants::HeaderSize;
        if (std::distance(begin, end) >= maxDataSize) {
            std::vector<char> data(begin, begin + maxDataSize);
            begin += maxDataSize;
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
        (void)begin;
        (void)end;
    }
}