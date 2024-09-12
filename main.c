#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#include "membermanager.h"

#define TCP_PORT 5100
#define MAX_CLIENTS 50

int pipefds[MAX_CLIENTS][2];
int clientSockets[MAX_CLIENTS];
int clientCount = 0;

void sigchld_handler(int signum) {
    while (waitpid(0, NULL, WNOHANG) > 0);
}

void handleParentProcess(int signo);
void handleChildProcess(int clientIdx); 

int main(int argc, char** argv) {
    int ssock;
    socklen_t clen;
    struct sockaddr_in servaddr, cliaddr;
    pid_t pid;
    char mesg[BUFSIZ];

    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    loadUsers("users.txt");
   
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

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

    signal(SIGUSR1, handleParentProcess);
    
    do {
        clen = sizeof(cliaddr);
        int csock = accept(ssock, (struct sockaddr*) &cliaddr, &clen);

        if (csock == -1) {
            perror("accept");
            continue;
        }

        if (pipe(pipefds[clientCount]) == -1) {
            perror("pipe");
            close(csock);
            continue;
        }

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

void handleParentProcess(int signo) {
    char buffer[BUFSIZ];
    int bytes;

    for (int i = 0; i < clientCount; i++) {
        bytes = read(pipefds[i][0], buffer, BUFSIZ);
        if (bytes > 0) {
            buffer[bytes] = '\0';
        }

        for (int j = 0; j < clientCount; j++) {
            if (j != i) {
                write(clientSockets[j], buffer, sizeof(buffer));
            }
        }
    }

}

void handleChildProcess(int clientIdx) {
    char buffer[BUFSIZ];
    int bytes;

    while (1) {
        bytes = read(clientSockets[clientIdx], buffer, BUFSIZ);
        if (bytes <= 0) {
            break;
        }

        buffer[bytes] = '\0';
        
        if (strncmp(buffer, "MESSAGE", 7) == 0) {
            char id[50], msg[1024], format[1024];
            sscanf(buffer, "MESSAGE:%s %[^\n]", id, msg);

            sprintf(format, "%s: %s", id, msg);

            write(pipefds[clientIdx][1], format, strlen(format));
            kill(getppid(), SIGUSR1);
        } else {
            char cmd[10], first[BUFSIZ], second[BUFSIZ];
            sscanf(buffer, "%[^:]:%s %s", cmd, first, second);
            int num;
            if (strcmp(cmd, "LOGIN") == 0) {
                num = loginUser(first, second);
                if (num == 1) printf("%s logged in.\n", first);
                write(clientSockets[clientIdx], &num, sizeof(int));
            } else if (strcmp(cmd, "SIGNUP") == 0) {
                num = signupUser(first, second);
                write(clientSockets[clientIdx], &num, sizeof(int));
            }
        }

        
    }

    close(pipefds[clientIdx][1]);
    close(clientSockets[clientIdx]);

    kill(getppid(), SIGUSR1);
}
