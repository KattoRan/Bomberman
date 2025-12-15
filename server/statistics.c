/* server/statistics.c - Player Statistics Tracking */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "../common/protocol.h"
#include "server.h"

extern sqlite3 *db;

// Record match completion and update statistics
int stats_record_match(int *player_ids, int *placements, int *kills, 
                       int num_players, int winner_id, int duration_seconds) {
    // Insert match history
    sqlite3_stmt *stmt;
    const char *match_sql = 
        "INSERT INTO MatchHistory (winner_id, duration_seconds, num_players) "
        "VALUES (?, ?, ?)";
    
    if (sqlite3_prepare_v2(db, match_sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "[STATS] Prepare failed: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    
    if (winner_id >= 0) {
        sqlite3_bind_int(stmt, 1, player_ids[winner_id]);
    } else {
        sqlite3_bind_null(stmt, 1);  // Draw
    }
    sqlite3_bind_int(stmt, 2, duration_seconds);
    sqlite3_bind_int(stmt, 3, num_players);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return -1;
    }
    sqlite3_finalize(stmt);
    
    int match_id = (int)sqlite3_last_insert_rowid(db);
    printf("[STATS] Recorded match %d (duration: %ds, players: %d)\n", 
           match_id, duration_seconds, num_players);
    
    // Update each player's statistics
    for (int i = 0; i < num_players; i++) {
        int user_id = player_ids[i];
        int won = (i == winner_id) ? 1 : 0;
        int player_kills = kills[i];
        int deaths = (placements[i] == 1) ? 0 : 1;  // Winner survives
        
        // Update Statistics table
        const char *update_sql = 
            "UPDATE Statistics SET "
            "total_matches = total_matches + 1, "
            "wins = wins + ?, "
            "total_kills = total_kills + ?, "
            "deaths = deaths + ? "
            "WHERE user_id = ?";
        
        if (sqlite3_prepare_v2(db, update_sql, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, won);
            sqlite3_bind_int(stmt, 2, player_kills);
            sqlite3_bind_int(stmt, 3, deaths);
            sqlite3_bind_int(stmt, 4, user_id);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        
        // Insert into MatchPlayers (ELO change will be added separately)
        const char *player_sql = 
            "INSERT INTO MatchPlayers (match_id, user_id, placement, kills, deaths) "
            "VALUES (?, ?, ?, ?, ?)";
        
        if (sqlite3_prepare_v2(db, player_sql, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, match_id);
            sqlite3_bind_int(stmt, 2, user_id);
            sqlite3_bind_int(stmt, 3, placements[i]);
            sqlite3_bind_int(stmt, 4, player_kills);
            sqlite3_bind_int(stmt, 5, deaths);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        
        printf("[STATS]   Player %d: Placement %d, Kills %d\n", 
               user_id, placements[i], player_kills);
    }
    
    return match_id;
}

// Get user statistics
int stats_get_profile(int user_id, ProfileData *out_profile) {
    sqlite3_stmt *stmt;
    const char *sql = 
        "SELECT u.username, u.display_name, u.elo_rating, "
        "       COALESCE(s.total_matches, 0), COALESCE(s.wins, 0), "
        "       COALESCE(s.total_kills, 0), COALESCE(s.deaths, 0) "
        "FROM Users u "
        "LEFT JOIN Statistics s ON u.id = s.user_id "
        "WHERE u.id = ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return -1;
    }
    
    if (out_profile) {
        strncpy(out_profile->username, 
                (const char *)sqlite3_column_text(stmt, 0), 
                MAX_USERNAME - 1);
        strncpy(out_profile->display_name, 
                (const char *)sqlite3_column_text(stmt, 1), 
                MAX_DISPLAY_NAME - 1);
        out_profile->elo_rating = sqlite3_column_int(stmt, 2);
        out_profile->tier = get_tier(out_profile->elo_rating);
        out_profile->total_matches = sqlite3_column_int(stmt, 3);
        out_profile->wins = sqlite3_column_int(stmt, 4);
        out_profile->total_kills = sqlite3_column_int(stmt, 5);
        out_profile->deaths = sqlite3_column_int(stmt, 6);
    }
    
    sqlite3_finalize(stmt);
    return 0;
}

// Get top N players for leaderboard
int stats_get_leaderboard(LeaderboardEntry *out_entries, int max_count) {
    sqlite3_stmt *stmt;
    const char *sql = 
        "SELECT u.display_name, u.elo_rating, COALESCE(s.wins, 0) "
        "FROM Users u "
        "LEFT JOIN Statistics s ON u.id = s.user_id "
        "ORDER BY u.elo_rating DESC "
        "LIMIT ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, max_count);
    
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_count) {
        out_entries[count].rank = count + 1;
        strncpy(out_entries[count].display_name, 
                (const char *)sqlite3_column_text(stmt, 0), 
                MAX_DISPLAY_NAME - 1);
        out_entries[count].elo_rating = sqlite3_column_int(stmt, 1);
        out_entries[count].wins = sqlite3_column_int(stmt, 2);
        count++;
    }
    
    sqlite3_finalize(stmt);
    printf("[STATS] Retrieved %d leaderboard entries\n", count);
    return count;
}

// Update bombs planted stat
void stats_increment_bombs(int user_id) {
    sqlite3_stmt *stmt;
    const char *sql = 
        "UPDATE Statistics SET bombs_planted = bombs_planted + 1 "
        "WHERE user_id = ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

// Update walls destroyed stat
void stats_increment_walls(int user_id, int count) {
    sqlite3_stmt *stmt;
    const char *sql = 
        "UPDATE Statistics SET walls_destroyed = walls_destroyed + ? "
        "WHERE user_id = ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, count);
        sqlite3_bind_int(stmt, 2, user_id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}
