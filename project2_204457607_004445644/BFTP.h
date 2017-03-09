#pragma once

#include <string>
#include <stdint.h>

#include "RDTP.h"

// Basic file transfer protocol
namespace BFTP
{
    class BFTPSession
    {
    public:
        BFTPSession(RDTP::RDTPConnection& rc);

        std::string ReceiveFilename();
        void SendFilename(const std::string& filename);

        void NotifyFileNotFound();
        bool WasFileFound();

        void SendFile(std::basic_istream<char>& is, size_t size);
        void ReceiveFile(std::basic_ostream<char>& os);

    private:
        RDTP::RDTPConnection& _rc;
    };
}