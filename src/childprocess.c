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

        if (strncmp(buffer, "MESSAGE", 7) == 0) {
            char id[50], msg[BUFSIZ], format[BUFSIZ];
            sscanf(buffer, "MESSAGE:%s %[^\n]", id, msg);

            if (strncmp(msg, "exit", 4) == 0) {
                break;
            }

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
