#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

#include "config.h"
#include "parentprocess.h"

extern int pipefds[MAX_CLIENTS][2];
extern int clientSockets[MAX_CLIENTS];
extern int clientCount;

void handleParentProcess(int signo) {
    char buffer[BUFSIZ];
    int bytes;

    for (int i = 0; i < clientCount; i++) {
        
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
            
            if (j != i) {
                int write_bytes = write(clientSockets[j], buffer, strlen(buffer));
                if (write_bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    continue;
                }
            }
        }
    }
}

void sigchld_handler(int signum) {
    while (waitpid(0, NULL, WNOHANG) > 0);
}

void setupParentProcessSignals() {
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
}
