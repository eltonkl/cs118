#include <iterator>
#include <istream>
#include <ostream>
#include <string>

#include "BFTP.h"
#include "RDTP.h"

using namespace std;
using namespace BFTP;
using namespace RDTP;

namespace BFTP
{
    BFTPSession::BFTPSession(RDTP::RDTPConnection& rc) :
        _rc(rc) {}

    std::string BFTPSession::ReceiveFilename()
    {
        return string();
    }

    void BFTPSession::SendFilename(const string& filename)
    {
        // http://stackoverflow.com/a/3407254
        size_t multiple = Constants::MaxPacketSize - Constants::HeaderSize;
        size_t toRound = filename.length() + 1;
        size_t remainder = toRound % multiple;
        size_t rounded = remainder == 0 ? toRound : toRound + multiple - remainder;

        vector<char> formattedFilename;
        formattedFilename.reserve(rounded);
        formattedFilename.insert(formattedFilename.begin(), filename.begin(), filename.end());
        formattedFilename.resize(rounded, '\0');
        _rc.Write(formattedFilename.begin(), formattedFilename.end());
    }

    void BFTPSession::NotifyFileNotFound()
    {

    }

    bool BFTPSession::WasFileFound()
    {
        return false;
    }

    void BFTPSession::SendFile(std::basic_istream<char>& is, size_t size)
    {
        (void)is;
        (void)size;
    }

    void BFTPSession::ReceiveFile(std::basic_ostream<char>& os)
    {
        (void)os;
    }
}