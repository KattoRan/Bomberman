/* server/friend_system.c - Friend Management System */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "../common/protocol.h"
#include "server.h"

extern sqlite3 *db;  // Defined in database.c

// Send friend request from user_id to target user (by display name)
int friend_send_request(int sender_id, const char *target_display_name) {
    // Find target user by display name
    User target;
    if (db_find_user_by_display_name(target_display_name, &target) != 0) {
        printf("[FRIEND] Target user not found: %s\n", target_display_name);
        return -1;  // User not found
    }
    
    if (target.id == sender_id) {
        printf("[FRIEND] Cannot send friend request to yourself\n");
        return -2;  // Cannot friend yourself
    }
    
    // Check if friendship already exists
    sqlite3_stmt *stmt;
    const char *check_sql = 
        "SELECT status FROM Friendships "
        "WHERE (user_id_1 = ? AND user_id_2 = ?) OR (user_id_1 = ? AND user_id_2 = ?)";
    
    if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "[FRIEND] Prepare failed: %s\n", sqlite3_errmsg(db));
        return -3;
    }
    
    sqlite3_bind_int(stmt, 1, sender_id);
    sqlite3_bind_int(stmt, 2, target.id);
    sqlite3_bind_int(stmt, 3, target.id);
    sqlite3_bind_int(stmt, 4, sender_id);
    
    int rc = sqlite3_step(stmt);
    
    if (rc == SQLITE_ROW) {
        const char *status = (const char *)sqlite3_column_text(stmt, 0);
        sqlite3_finalize(stmt);
        
        if (strcmp(status, "ACCEPTED") == 0) {
            printf("[FRIEND] Already friends\n");
            return -4;  // Already friends
        } else {
            printf("[FRIEND] Friend request already pending\n");
            return -5;  // Request already pending
        }
    }
    sqlite3_finalize(stmt);
    
    // Create new friend request
    const char *insert_sql = 
        "INSERT INTO Friendships (user_id_1, user_id_2, status) VALUES (?, ?, 'PENDING')";
    
    if (sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "[FRIEND] Prepare failed: %s\n", sqlite3_errmsg(db));
        return -3;
    }
    
    sqlite3_bind_int(stmt, 1, sender_id);
    sqlite3_bind_int(stmt, 2, target.id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "[FRIEND] Insert failed: %s\n", sqlite3_errmsg(db));
        return -3;
    }
    
    User sender;
    db_get_user_by_id(sender_id, &sender);
    printf("[FRIEND] Friend request sent: %s -> %s\n", 
           sender.display_name, target.display_name);
    return 0;
}

// Accept friend request
int friend_accept_request(int user_id, int requester_id) {
    sqlite3_stmt *stmt;
    const char *sql = 
        "UPDATE Friendships SET status = 'ACCEPTED', accepted_at = CURRENT_TIMESTAMP "
        "WHERE user_id_1 = ? AND user_id_2 = ? AND status = 'PENDING'";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, requester_id);
    sqlite3_bind_int(stmt, 2, user_id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        return -1;
    }
    
    printf("[FRIEND] Friend request accepted: User %d accepted User %d\n", 
           user_id, requester_id);
    return 0;
}

// Decline/remove friend request
int friend_decline_request(int user_id, int requester_id) {
    sqlite3_stmt *stmt;
    const char *sql = 
        "DELETE FROM Friendships "
        "WHERE user_id_1 = ? AND user_id_2 = ? AND status = 'PENDING'";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, requester_id);
    sqlite3_bind_int(stmt, 2, user_id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE || sqlite3_changes(db) == 0) {
        return -1;
    }
    
    printf("[FRIEND] Friend request declined: User %d declined User %d\n", 
           user_id, requester_id);
    return 0;
}

// Remove friend (delete friendship)
int friend_remove(int user_id, int friend_id) {
    sqlite3_stmt *stmt;
    const char *sql = 
        "DELETE FROM Friendships "
        "WHERE ((user_id_1 = ? AND user_id_2 = ?) OR (user_id_1 = ? AND user_id_2 = ?)) "
        "AND status = 'ACCEPTED'";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, friend_id);
    sqlite3_bind_int(stmt, 3, friend_id);
    sqlite3_bind_int(stmt, 4, user_id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE || sqlite3_changes(db) == 0) {
        return -1;
    }
    
    printf("[FRIEND] Friendship removed: User %d removed User %d\n", 
           user_id, friend_id);
    return 0;
}

// Get friends list with online status
int friend_get_list(int user_id, FriendInfo *out_friends, int max_count) {
    sqlite3_stmt *stmt;
    const char *sql = 
        "SELECT u.id, u.display_name, u.elo_rating "
        "FROM Friendships f "
        "JOIN Users u ON (f.user_id_1 = u.id OR f.user_id_2 = u.id) "
        "WHERE ((f.user_id_1 = ? OR f.user_id_2 = ?) AND u.id != ?) "
        "AND f.status = 'ACCEPTED' "
        "ORDER BY u.display_name";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "[FRIEND] Prepare failed: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_bind_int(stmt, 3, user_id);
    
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_count) {
        out_friends[count].user_id = sqlite3_column_int(stmt, 0);
        strncpy(out_friends[count].display_name, 
                (const char *)sqlite3_column_text(stmt, 1), 
                MAX_DISPLAY_NAME - 1);
        out_friends[count].elo_rating = sqlite3_column_int(stmt, 2);
        
        // TODO: Get online status from active connections
        // For now, default to offline
        out_friends[count].is_online = 0;
        
        count++;
    }
    
    sqlite3_finalize(stmt);
    printf("[FRIEND] Retrieved %d friends for user %d\n", count, user_id);
    return count;
}

// Get pending friend requests (incoming)
int friend_get_pending_requests(int user_id, FriendInfo *out_requests, int max_count) {
    sqlite3_stmt *stmt;
    const char *sql = 
        "SELECT u.id, u.display_name, u.elo_rating "
        "FROM Friendships f "
        "JOIN Users u ON f.user_id_1 = u.id "
        "WHERE f.user_id_2 = ? AND f.status = 'PENDING' "
        "ORDER BY f.requested_at DESC";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_count) {
        out_requests[count].user_id = sqlite3_column_int(stmt, 0);
        strncpy(out_requests[count].display_name, 
                (const char *)sqlite3_column_text(stmt, 1), 
                MAX_DISPLAY_NAME - 1);
        out_requests[count].elo_rating = sqlite3_column_int(stmt, 2);
        out_requests[count].is_online = 0;  // Not relevant for pending requests
        count++;
    }
    
    sqlite3_finalize(stmt);
    printf("[FRIEND] Retrieved %d pending requests for user %d\n", count, user_id);
    return count;
}
