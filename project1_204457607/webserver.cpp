/* Based on ClientServer_Example code */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>
#include <signal.h>
#include <string>
#include <unistd.h>

using namespace std;

void sigchld_handler(int s)
{
    (void)s;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void error(string msg)
{
    perror(msg.c_str());
    exit(1);
}

void respondToClient(int sockfd);

int main(int argc, char** argv)
{
    int sockfd, newsockfd, portno, pid;
    socklen_t cli_len;
    struct sockaddr_in serv_addr, cli_addr;
    struct sigaction sa;

    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 5);

    cli_len = sizeof(cli_addr);

    // Kill zombie processes
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, nullptr) == -1)
        error("sigaction");

    while (true)
    {
        newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &cli_len);

        if (newsockfd < 0)
            error("ERROR on accept");

        pid = fork();
        if (pid < 0)
            error("ERROR on fork");

        if (pid == 0)
        {
            close(sockfd);
            respondToClient(newsockfd);
            close(newsockfd);
            exit(0);
        }
        else
            close(newsockfd);
    }

    return 0;
}

void respondToClient(int sockfd)
{
    int n;
    char buffer[256] = {0};
    string response;

    n = read(sockfd, buffer, 255);
    if (n < 0)
        error("ERROR reading from socket");
    
    response.append(buffer, n);
    printf("%s\n", response.c_str());
    
    n = write(sockfd, "I got your message", 18);
    if (n < 0)
        error ("ERROR writing to socket");
}
