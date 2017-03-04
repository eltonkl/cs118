#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "RDTP.h"
#include "Printer.h"

using namespace std;
using namespace RDTP;

void sigchld_handler(int s)
{
    (void)s;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// Print error message and then exit
void error(string msg)
{
    perror(msg.c_str());
    exit(1);
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
}