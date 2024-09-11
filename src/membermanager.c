#include <stdio.h>
#include <string.h>

#include "membermanager.h"

User users[MAX_USERS];
int userCount = 0;

void loadUsers(const char* filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) return;

    while (fscanf(file, "%s %s", users[userCount].id, users[userCount].password) == 2) userCount++;

    fclose(file);
}

void saveUsers(const char* filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) return;

    for (int i = 0; i < userCount; i++) {
        fprintf(file, "%s %s\n", users[i].id, users[i].password);
    }

    fclose(file);
}

int loginUser(const char* id, const char* password) {
    for (int i = 0; i < userCount; i++) {
        if (strcmp(users[i].id, id) == 0 && strcmp(users[i].password, password) == 0)
            return 1;
    }
    return 0;
}

int signupUser(const char* id, const char* password) {
    for (int i = 0; i < userCount; i++) {
        if (strcmp(users[i].id, id) == 0) return 0;
    }

    strcpy(users[userCount].id, id);
    strcpy(users[userCount].password, password);
    userCount++;

    saveUsers("users.txt");
    return 1;
}
