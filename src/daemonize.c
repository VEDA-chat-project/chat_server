#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "config.h"

void daemonize() {
    pid_t pid;

    pid = fork();
    if (pid < 0) {
        perror("first fork()");
        exit(1);
    }

    if (pid > 0) {
        exit(0);
    }

    if (setsid() < 0) {
        perror("setsid()");
        exit(1);
    }

    pid = fork();
    if (pid < 0) {
        perror("second fork()");
        exit(1);
    }

    if (pid > 0) {
        exit(0);
    }

    if (chdir("/") < 0) {
        perror("chdir");
        exit(1);
    }

    umask(0);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);
}
