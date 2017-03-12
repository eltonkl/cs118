#include <iterator>
#include <istream>
#include <ostream>
#include <string>
#include <sstream>
#include <iomanip>

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
        ostringstream oss;
        _rc.Read(oss, 256);

        return oss.str();
    }

    // Maximum filename size is 255 characters
    void BFTPSession::SendFilename(const string& filename)
    {
        vector<char> formattedFilename;

        formattedFilename.reserve(256);
        formattedFilename.insert(formattedFilename.begin(), filename.begin(), filename.end());
        formattedFilename.resize(256, '\0');
        formattedFilename[255] = '\0';

        _rc.Write(formattedFilename.begin(), formattedFilename.end());
    }

    void BFTPSession::NotifyFileNotFound()
    {
        string data = "0";
        _rc.Write(data.begin(), data.end());
    }

    bool BFTPSession::WasFileFound()
    {
        ostringstream oss;
        _rc.Read(oss, 1);

        if (oss.str() == "0")
            return false;
        else
            return true;
    }

    void BFTPSession::SendFile(std::basic_istream<char>& is, size_t size)
    {
        stringstream ss;
        ss << '1' << setfill('0') << setw(20) << size;

        _rc << ss << is;
    }

    void BFTPSession::ReceiveFile(std::basic_ostream<char>& os)
    {
        stringstream ss;
        _rc.Read(ss, 20);

        size_t size;
        ss >> size;
        _rc.Read(os, size);
    }
}