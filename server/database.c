/* server/database.c - SQLite3 Implementation */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sqlite3.h>
#include "../common/protocol.h"
#include "server.h"

#define DB_FILE "bomberman.db"
#define SCHEMA_FILE "server/schema.sql"

sqlite3 *db = NULL;  // Exposed for other modules

// Simple password hashing with salt (SHA-256 would be better in production)
void generate_salt(char *salt, size_t len) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t i = 0; i < len - 1; i++) {
        salt[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    salt[len - 1] = '\0';
}

void hash_password(const char *password, const char *salt, char *output) {
    // Simple hash: concatenate password + salt and hash
    // In production, use bcrypt or proper SHA-256
    unsigned long hash = 5381;
    char combined[512];
    snprintf(combined, sizeof(combined), "%s%s", password, salt);
    
    const char *str = combined;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    snprintf(output, MAX_PASSWORD, "%lu", hash);
}

// Initialize database and create tables from schema
int db_init() {
    int rc = sqlite3_open(DB_FILE, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DB] Cannot open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    
    printf("[DB] SQLite database opened: %s\n", DB_FILE);
    
    // Read and execute schema file
    FILE *f = fopen(SCHEMA_FILE, "r");
    if (!f) {
        fprintf(stderr, "[DB] Cannot open schema file: %s\n", SCHEMA_FILE);
        return -1;
    }
    
    // Read schema file
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *schema = malloc(fsize + 1);
    fread(schema, 1, fsize, f);
    fclose(f);
    schema[fsize] = '\0';
    
    // Execute schema
    char *err_msg = NULL;
    rc = sqlite3_exec(db, schema, NULL, NULL, &err_msg);
    free(schema);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DB] Schema execution failed: %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }
    
    printf("[DB] Database initialized successfully\n");
    srand(time(NULL)); // For salt generation
    return 0;
}

void db_close() {
    if (db) {
        sqlite3_close(db);
        printf("[DB] Database closed\n");
    }
}

// Validate username (3-31 chars, alphanumeric + underscore)
int validate_username(const char *username) {
    int len = strlen(username);
    if (len < 3 || len > MAX_USERNAME - 1) return 0;
    for (int i = 0; i < len; i++) {
        if (!isalnum(username[i]) && username[i] != '_') return 0;
    }
    return 1;
}

// Validate email (basic check for @ and .)
int validate_email(const char *email) {
    int len = strlen(email);
    if (len < 5 || len > 127) return 0;
    
    int has_at = 0, has_dot_after_at = 0;
    for (int i = 0; i < len; i++) {
        if (email[i] == '@') {
            if (has_at) return 0; // Multiple @
            has_at = 1;
        }
        if (has_at && email[i] == '.') {
            has_dot_after_at = 1;
        }
    }
    return has_at && has_dot_after_at;
}

// Register new user
int db_register_user(const char *username, const char *email, const char *password) {
    if (!validate_username(username)) {
        printf("[DB] Invalid username format: %s\n", username);
        return AUTH_INVALID;
    }
    
    if (!validate_email(email)) {
        printf("[DB] Invalid email format: %s\n", email);
        return AUTH_INVALID;
    }
    
    if (strlen(password) < 4) {
        printf("[DB] Password too short\n");
        return AUTH_INVALID;
    }
    
    // Check if username/email already exists
    int username_exists = 0;
    int email_exists = 0;

    sqlite3_stmt *stmt;
    const char *check_user_sql = "SELECT id FROM Users WHERE username = ?";
    
    if (sqlite3_prepare_v2(db, check_user_sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "[DB] Prepare failed: %s\n", sqlite3_errmsg(db));
        return AUTH_FAILED;
    }
    
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc == SQLITE_ROW) {
        username_exists = 1;
    }
    
    // Check if email already exists
    const char *check_email_sql = "SELECT id FROM Users WHERE email = ?";
    
    if (sqlite3_prepare_v2(db, check_email_sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "[DB] Prepare failed: %s\n", sqlite3_errmsg(db));
        return AUTH_FAILED;
    }
    
    sqlite3_bind_text(stmt, 1, email, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc == SQLITE_ROW) {
        email_exists = 1;
    }

    if (username_exists && email_exists) {
        printf("[DB] Username and email already exist\n");
        return AUTH_USER_EXISTS;
    }

    if (username_exists) {
        printf("[DB] Username already exists\n");
        return AUTH_USERNAME_EXISTS;
    }

    if (email_exists) {
        printf("[DB] Email already exists\n");
        return AUTH_EMAIL_EXISTS;
    }
    
    // Generate salt and hash password
    char salt[32];
    char hash[MAX_PASSWORD];
    generate_salt(salt, sizeof(salt));
    hash_password(password, salt, hash);
    
    // Insert new user (display_name starts same as username)
    const char *insert_sql = 
        "INSERT INTO Users (username, display_name, email, password_hash, salt) "
        "VALUES (?, ?, ?, ?, ?)";
    
    if (sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "[DB] Prepare failed: %s\n", sqlite3_errmsg(db));
        return AUTH_FAILED;
    }
    
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, username, -1, SQLITE_STATIC); // Initial display_name = username
    sqlite3_bind_text(stmt, 3, email, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, hash, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, salt, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "[DB] Insert failed: %s\n", sqlite3_errmsg(db));
        return AUTH_FAILED;
    }
    
    // Create default statistics record
    int user_id = (int)sqlite3_last_insert_rowid(db);
    const char *stats_sql = "INSERT INTO Statistics (user_id) VALUES (?)";
    
    if (sqlite3_prepare_v2(db, stats_sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    
    printf("[DB] Registered user: %s (email: %s, id: %d)\n", username, email, user_id);
    return AUTH_SUCCESS;
}

// Login user (by username or email)
int db_login_user(const char *identifier, const char *password, User *out_user) {
    sqlite3_stmt *stmt;
    const char *sql = 
        "SELECT id, username, display_name, email, password_hash, salt, elo_rating "
        "FROM Users WHERE username = ? OR email = ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "[DB] Prepare failed: %s\n", sqlite3_errmsg(db));
        return AUTH_FAILED;
    }
    
    sqlite3_bind_text(stmt, 1, identifier, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, identifier, -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        printf("[DB] User not found: %s\n", identifier);
        return AUTH_INVALID;
    }
    
    // Get stored hash and salt
    const char *stored_hash = (const char *)sqlite3_column_text(stmt, 4);
    const char *salt = (const char *)sqlite3_column_text(stmt, 5);
    
    // Hash provided password with stored salt
    char computed_hash[MAX_PASSWORD];
    hash_password(password, salt, computed_hash);
    
    // Compare hashes
    if (strcmp(stored_hash, computed_hash) != 0) {
        sqlite3_finalize(stmt);
        printf("[DB] Invalid password for: %s\n", identifier);
        return AUTH_INVALID;
    }
    
    // Populate user struct
    if (out_user) {
        out_user->id = sqlite3_column_int(stmt, 0);
        strncpy(out_user->username, (const char *)sqlite3_column_text(stmt, 1), MAX_USERNAME - 1);
        strncpy(out_user->display_name, (const char *)sqlite3_column_text(stmt, 2), MAX_DISPLAY_NAME - 1);
        strncpy(out_user->email, (const char *)sqlite3_column_text(stmt, 3), MAX_EMAIL - 1);
        out_user->elo_rating = sqlite3_column_int(stmt, 6);
        out_user->is_online = 1;
        out_user->lobby_id = -1;
    }
    
    sqlite3_finalize(stmt);
    
    // Update last_login
    const char *update_sql = "UPDATE Users SET last_login = CURRENT_TIMESTAMP WHERE id = ?";
    if (sqlite3_prepare_v2(db, update_sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, out_user->id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    
    printf("[DB] Login successful: %s (id: %d, ELO: %d)\n", 
           out_user->username, out_user->id, out_user->elo_rating);
    return AUTH_SUCCESS;
}

// Update user's display name
int db_update_display_name(int user_id, const char *new_display_name) {
    if (strlen(new_display_name) < 3 || strlen(new_display_name) > MAX_DISPLAY_NAME - 1) {
        return -1;
    }
    
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE Users SET display_name = ? WHERE id = ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, new_display_name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, user_id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc == SQLITE_DONE) {
        printf("[DB] Updated display name for user %d: %s\n", user_id, new_display_name);
        return 0;
    }
    return -1;
}

// Get user by ID
int db_get_user_by_id(int user_id, User *out_user) {
    sqlite3_stmt *stmt;
    const char *sql = 
        "SELECT id, username, display_name, email, elo_rating "
        "FROM Users WHERE id = ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    
    int rc = sqlite3_step(stmt);
    
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return -1;
    }
    
    if (out_user) {
        out_user->id = sqlite3_column_int(stmt, 0);
        strncpy(out_user->username, (const char *)sqlite3_column_text(stmt, 1), MAX_USERNAME - 1);
        strncpy(out_user->display_name, (const char *)sqlite3_column_text(stmt, 2), MAX_DISPLAY_NAME - 1);
        strncpy(out_user->email, (const char *)sqlite3_column_text(stmt, 3), MAX_EMAIL - 1);
        out_user->elo_rating = sqlite3_column_int(stmt, 4);
    }
    
    sqlite3_finalize(stmt);
    return 0;
}

// Find user by display name (for friend requests)
int db_find_user_by_display_name(const char *display_name, User *out_user) {
    sqlite3_stmt *stmt;
    const char *sql = 
        "SELECT id, username, display_name, email, elo_rating "
        "FROM Users WHERE display_name = ? COLLATE NOCASE";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, display_name, -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return -1;
    }
    
    if (out_user) {
        out_user->id = sqlite3_column_int(stmt, 0);
        strncpy(out_user->username, (const char *)sqlite3_column_text(stmt, 1), MAX_USERNAME - 1);
        strncpy(out_user->display_name, (const char *)sqlite3_column_text(stmt, 2), MAX_DISPLAY_NAME - 1);
        strncpy(out_user->email, (const char *)sqlite3_column_text(stmt, 3), MAX_EMAIL - 1);
        out_user->elo_rating = sqlite3_column_int(stmt, 4);
    }
    
    sqlite3_finalize(stmt);
    return 0;
}

// Update user's ELO rating
int db_update_elo(int user_id, int new_elo) {
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE Users SET elo_rating = ? WHERE id = ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, new_elo);
    sqlite3_bind_int(stmt, 2, user_id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? 0 : -1;
}
