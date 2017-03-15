#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>

#include "RDTP.h"
#include "BFTP.h"

using namespace std;
using namespace RDTP;
using namespace BFTP;

// Print error message and then exit
void error(string msg)
{
    perror(msg.c_str());
    exit(1);
}

// TODO: Fix crash if server isn't running and this program is run

int main(int argc, char** argv)
{
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    struct hostent *he;

    if (argc < 4)
    {
        fprintf(stderr, "ERROR, not enough arguments\n");
        exit(1);
    }

    he = gethostbyname(argv[1]);
    if (!he)
    {
        fprintf(stderr, "ERROR, unable to obtain address of %s\n", argv[1]);
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    memset(&serv_addr, '\0', sizeof(serv_addr));
    portno = atoi(argv[2]);
    serv_addr.sin_family = AF_INET;
    memcpy((void *)&serv_addr.sin_addr, he->h_addr_list[0], he->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR using connect");

    {
        cout << "Initiating RDTP connection as client." << endl;
        RDTPConnection rc(ApplicationType::Client, sockfd);
        if (!rc.IsConnectionEstablished())
            goto failure;

        {
            string filename(argv[3]);
            cout << "Requesting file " << filename << endl;
            BFTPSession bs(rc);
            bs.SendFilename(filename);

            if (bs.WasFileFound())
            {
                cout << "File found. Writing to ./received.data" << endl;
                ofstream ofs("./received.data", ios::binary | ios::trunc);

                bs.ReceiveFile(ofs);
            }
            else
                cout << "File not found. Exiting." << endl;
        }
        cout << "Closing RDTP connection." << endl;
        // Need destructor to fire before close(sockfd)
    }

failure:
    close(sockfd);
    return 0;
}