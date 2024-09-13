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
#include "membermanager.h"

extern int pipefds[MAX_CLIENTS][2];
extern int clientSockets[MAX_CLIENTS];
extern int clientCount;

void handleParentProcess(int signo) {
    char buffer[BUFSIZ];
    int bytes;

    for (int i = 0; i < clientCount; i++) { // for each file descriptor,
        bytes = read(pipefds[i][0], buffer, BUFSIZ); // check if a message has arrived.
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

        /*
         * if a message has arrived,
         * send the message to other clients via socket.
         */
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

void handleSignup(int signo) {
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

        int clientIdx;
        char id[50], password[50];
        int result = sscanf(buffer, "%d %s %s", &clientIdx, id, password);

        if (result != 3) continue;

        int num = signupUser(id, password); // Sign up (success : 1), (fail : 0)
        if (num == 1) printf("%s signed up.\n", id); // Sign up success
        
        if (write(clientSockets[clientIdx], &num, sizeof(int)) == -1) { // Send result to client
            perror("write to client failed");
        }
    }
}

// prevent zombie process
void sigchld_handler(int signum) {
    while (waitpid(0, NULL, WNOHANG) > 0) {
        if (clientCount > 0) clientCount--;
    }
}

// set up signal actions for SIGCHLD, SIGUSR1
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

    struct sigaction sa_usr2;
    sa_usr2.sa_handler = handleSignup;
    sigfillset(&sa_usr2.sa_mask);
    sa_usr2.sa_flags = SA_RESTART;

    if (sigaction(SIGUSR2, &sa_usr2, NULL) == -1) {
        perror("sigaction SIGUSR2");
        exit(1);
    }
}
