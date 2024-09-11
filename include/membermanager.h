#ifndef MEMBERMANAGER_H
#define MEMBERMANAGER_H

#define MAX_USERS 50

typedef struct {
    char id[50], password[50];
} User;

extern User users[MAX_USERS];
extern int userCount;

void loadUsers(const char*);
void saveUsers(const char*);
int loginUser(const char*, const char*);
int signupUser(const char*, const char*);
void handleMessage(int csock, const char*);

#endif
