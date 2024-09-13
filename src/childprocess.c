#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "config.h"
#include "membermanager.h"
#include "childprocess.h"

extern int pipefds[MAX_CLIENTS][2];
extern int clientSockets[MAX_CLIENTS];

void handleChildProcess(int clientIdx) {
    char buffer[BUFSIZ];
    int bytes;

    while (1) {
        // read a message from chat_client
        bytes = read(clientSockets[clientIdx], buffer, BUFSIZ);
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
         * buffer format
         * 1. "MESSAGE:id message"
         * 2. "LOGIN:id password"
         * 3. "SIGNUP:id password"
         */
        if (strncmp(buffer, "MESSAGE", 7) == 0) {
            char id[50], msg[BUFSIZ], format[BUFSIZ];
            sscanf(buffer, "MESSAGE:%s %[^\n]", id, msg);

            if (strncmp(msg, "exit", 4) == 0) {
                printf("%s logged out.\n", id);
                break;
            }

            sprintf(format, "%s: %s", id, msg);
            printf("%s\n", format);

            // send a message via pipe
            write(pipefds[clientIdx][1], format, strlen(format));
            kill(getppid(), SIGUSR1);
        } else { // Manage members -> See membermanager.c code
            char cmd[10], first[BUFSIZ], second[BUFSIZ];
            sscanf(buffer, "%[^:]:%s %s", cmd, first, second);
            int num;
            if (strcmp(cmd, "LOGIN") == 0) {
                num = loginUser(first, second); // Log in (success : 1), (fail : 0)
                if (num == 1) printf("%s logged in.\n", first); // Log in success
                write(clientSockets[clientIdx], &num, sizeof(int)); // Send result to client
            } else if (strcmp(cmd, "SIGNUP") == 0) {
                num = signupUser(first, second); // Sign up (success : 1), (fail : 0)
                if (num == 1) printf("%s signed up.\n", first); // Sign up success
                write(clientSockets[clientIdx], &num, sizeof(int)); // Send result to client
            }
        }
    }

    close(pipefds[clientIdx][1]);
    close(clientSockets[clientIdx]);
}
