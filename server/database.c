/* server/database.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../common/protocol.h"
#include "server.h"  // <--- THÊM DÒNG NÀY

#define USER_DB_FILE "users.dat"

User user_database[MAX_USERS];
int user_count = 0;

void simple_hash(const char *password, char *output) {
    unsigned long hash = 5381;
    int c;
    const char *str = password;
    while ((c = *str++)) hash = ((hash << 5) + hash) + c;
    snprintf(output, MAX_PASSWORD, "%lu", hash);
}

void load_users() {
    FILE *f = fopen(USER_DB_FILE, "rb");
    if (f) {
        fread(&user_count, sizeof(int), 1, f);
        fread(user_database, sizeof(User), user_count, f);
        fclose(f);
        printf("[DB] Loaded %d users\n", user_count);
    } else {
        printf("[DB] No database, starting fresh\n");
        user_count = 0;
    }
}

void save_users() {
    FILE *f = fopen(USER_DB_FILE, "wb");
    if (f) {
        fwrite(&user_count, sizeof(int), 1, f);
        fwrite(user_database, sizeof(User), user_count, f);
        fclose(f);
    }
}

User* find_user(const char *username) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(user_database[i].username, username) == 0) {
            return &user_database[i];
        }
    }
    return NULL;
}

int validate_username(const char *username) {
    int len = strlen(username);
    if (len < 3 || len > MAX_USERNAME - 1) return 0;
    for (int i = 0; i < len; i++) {
        if (!isalnum(username[i]) && username[i] != '_') return 0;
    }
    return 1;
}

int register_user(const char *username, const char *password) {
    if (!validate_username(username)) return 1;
    if (strlen(password) < 4) return 1;
    if (find_user(username) != NULL) return 2;
    if (user_count >= MAX_USERS) return 1;
    
    User *new_user = &user_database[user_count];
    strncpy(new_user->username, username, MAX_USERNAME - 1);
    simple_hash(password, new_user->password);
    new_user->is_online = 0;
    new_user->lobby_id = -1;
    
    user_count++;
    save_users();
    printf("[DB] Registered: %s\n", username);
    return 0;
}

int login_user(const char *username, const char *password) {
    User *user = find_user(username);
    if (!user) return 3;
    char hashed[MAX_PASSWORD];
    simple_hash(password, hashed);
    if (strcmp(user->password, hashed) != 0) return 3;
    user->is_online = 1;
    printf("[DB] Login: %s\n", username);
    return 0;
}

void logout_user(const char *username) {
    User *user = find_user(username);
    if (user) {
        user->is_online = 0;
        user->lobby_id = -1;
        printf("[DB] Logout: %s\n", username);
    }
}