#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#include "membermanager.h"

#define TCP_PORT 5100

void sigchld_handler(int signum) {
    while (waitpid(0, NULL, WNOHANG) > 0);
}

void handleClientMessage(int csock, const char* message);

int main(int argc, char** argv) {
    int ssock;
    socklen_t clen;
    struct sockaddr_in servaddr, cliaddr;
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

    if (listen(ssock, 8) < 0) {
        perror("listen()");
        exit(1);
    }



    clen = sizeof(cliaddr);
    do {
        int n, csock = accept(ssock, (struct sockaddr*) &cliaddr, &clen);

        if (fork() == 0) { // child process
            inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ);
            printf("Client is connected : %s\n", mesg);

            while (1) {
                if ((n = read(csock, mesg, BUFSIZ)) <= 0) {
                    if (n == 0) {
                        close(csock);
                        break;
                    }
                }

                handleClientMessage(csock, mesg);
                
                if (strncmp(mesg, "exit", 4) == 0) {
                    close(csock);
                    break;
                }
            }

            exit(0);
        }

    } while(1);

    close(ssock);

    return 0;
}

void handleClientMessage(int csock, const char* message) {
    char cmd[10], id[50], password[50];
    sscanf(message, "%[^:]:%s %s", cmd, id, password);

    if (strcmp(cmd, "LOGIN") == 0) {
        if (loginUser(id, password)) {
            write(csock, "LOGIN_SUCCESS", strlen("LOGIN_SUCCESS"));
        } else {
            write(csock, "LOGIN_FAIL", strlen("LOGIN_FAIL"));
        }
    } else if (strcmp(cmd, "SIGNUP") == 0) {
        if (signupUser(id, password)) {
            write(csock, "SIGNUP_SUCCESS", strlen("SIGNUP_SUCCESS"));
        } else {
            write(csock, "SIGNUP_FAIL", strlen("SIGNUP_FAIL"));
        }
    }
}
