#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#include "config.h"
#include "membermanager.h"
#include "parentprocess.h"
#include "childprocess.h"

int pipefds[MAX_CLIENTS][2];
int clientSockets[MAX_CLIENTS];
int clientCount = 0;

int main(int argc, char** argv) {
    int ssock;
    socklen_t clen;
    struct sockaddr_in servaddr, cliaddr;
    pid_t pid;
    char mesg[BUFSIZ];

    setupParentProcessSignals();

    loadUsers("users.txt");

    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(1);
    }

    int yes = 1;
    if (setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(TCP_PORT);

    if (bind(ssock, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) {
        perror("bind()");
        exit(1);
    }

    if (listen(ssock, 50) < 0) {
        perror("listen()");
        exit(1);
    }

    do {
        clen = sizeof(cliaddr);
        int csock = accept(ssock, (struct sockaddr*) &cliaddr, &clen);

        if (csock == -1) {
            perror("accept");
            continue;
        }

        int socketFlags = fcntl(csock, F_GETFL, 0);
        fcntl(csock, F_SETFL, socketFlags | O_NONBLOCK);
        
        if (pipe(pipefds[clientCount]) == -1) {
            perror("pipe");
            close(csock);
            continue;
        }

        int pipeFlags = fcntl(pipefds[clientCount][0], F_GETFL, 0);
        fcntl(pipefds[clientCount][0], F_SETFL, pipeFlags | O_NONBLOCK);

        clientSockets[clientCount] = csock;
        
        pid = fork();

        if (pid == -1) {
            perror("fork");
            close(csock);
            continue;
        }

        if (pid == 0) { // child process
            inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ);
            printf("Client is connected : %s\n", mesg);

            close(pipefds[clientCount][0]);
            handleChildProcess(clientCount);
            close(csock);
            exit(0);
        } else { // parent process
            close(pipefds[clientCount++][1]);
        }

    } while(1);

    close(ssock);

    return 0;
}
