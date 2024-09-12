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

    struct sigaction sa_chld;
    sa_chld.sa_handler = sigchld_handler;
    sigfillset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART;
   
    if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
        perror("sigaction SIGCHLD");
        exit(1);
    }

    struct sigaction sa_usr1;
    sa_usr1.sa_handler = handleParentProcess;
    sigfillset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = SA_RESTART;

    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1) {
        perror("sigaction SIGUSR1");
        exit(1);
    }

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
    printf("%d\n", clientCount);

    for (int i = 0; i < clientCount; i++) {
        int pipeFlags = fcntl(pipefds[i][0], F_GETFL, 0);
        fcntl(pipefds[i][0], F_SETFL, pipeFlags | O_NONBLOCK);

        bytes = read(pipefds[i][0], buffer, BUFSIZ);
        if (bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            } else {
                perror("read");
                break;
            }
        } else if (bytes > 0) {
            buffer[bytes] = '\0';
        }

        for (int j = 0; j < clientCount; j++) {
            int socketFlags = fcntl(clientSockets[j], F_GETFL, 0);
            fcntl(clientSockets[j], F_SETFL, socketFlags | O_NONBLOCK);

            if (j != i) {
                int write_bytes = write(clientSockets[j], buffer, strlen(buffer));
                if (write_bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    continue;
                }
            }

            fcntl(clientSockets[j], F_SETFL, socketFlags & ~O_NONBLOCK);
        }

        fcntl(pipefds[i][0], F_SETFL, pipeFlags & ~O_NONBLOCK);
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
            char id[50], msg[BUFSIZ], format[BUFSIZ];
            sscanf(buffer, "MESSAGE:%s %[^\n]", id, msg);

            sprintf(format, "%s: %s", id, msg);
            printf("%s\n", format);

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
                if (num == 1) printf("%s signed up.\n", first);
                write(clientSockets[clientIdx], &num, sizeof(int));
            }
        }
    }

    close(pipefds[clientIdx][1]);
    close(clientSockets[clientIdx]);
}
