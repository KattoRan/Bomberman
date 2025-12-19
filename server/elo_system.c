/* server/elo_system.c - ELO Rating System */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sqlite3.h>
#include "../common/protocol.h"
#include "server.h"

extern sqlite3 *db;

// Get K-factor based on number of matches played
// New players have higher K for faster rating adjustment
int get_k_factor(int matches_played) {
    if (matches_played < 10) return 40;      // New players
    if (matches_played < 30) return 30;      // Intermediate
    return 20;                                // Experienced players
}

// Calculate expected score based on ELO difference
double calculate_expected_score(int rating_a, int rating_b) {
    return 1.0 / (1.0 + pow(10.0, (rating_b - rating_a) / 400.0));
}

// Calculate ELO change for a player
// placement: 1 = winner, 2-4 = losers (equal treatment for now)
int calculate_elo_change(int player_rating, int avg_opponent_rating, 
                         int placement, int matches_played) {
    double expected = calculate_expected_score(player_rating, avg_opponent_rating);
    double actual = (placement == 1) ? 1.0 : 0.0;  // 1 for win, 0 for loss
    
    int k = get_k_factor(matches_played);
    int change = (int)(k * (actual - expected));
    
    return change;
}

// Update ELO ratings for all players in a match
// Returns 0 on success, -1 on failure
int elo_update_after_match(int *player_ids, int *placements, int num_players, int *out_elo_changes) {
    if (num_players < 2) return -1;
    
    // Get current ratings and match counts
    int ratings[MAX_CLIENTS];
    int match_counts[MAX_CLIENTS];
    
    for (int i = 0; i < num_players; i++) {
        sqlite3_stmt *stmt;
        const char *sql = 
            "SELECT u.elo_rating, COALESCE(s.total_matches, 0) "
            "FROM Users u "
            "LEFT JOIN Statistics s ON u.id = s.user_id "
            "WHERE u.id = ?";
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
            return -1;
        }
        
        sqlite3_bind_int(stmt, 1, player_ids[i]);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            ratings[i] = sqlite3_column_int(stmt, 0);
            match_counts[i] = sqlite3_column_int(stmt, 1);
        } else {
            sqlite3_finalize(stmt);
            return -1;
        }
        sqlite3_finalize(stmt);
    }
    
    // Calculate average opponent rating for each player
    int total_rating = 0;
    for (int i = 0; i < num_players; i++) {
        total_rating += ratings[i];
    }
    
    // Update each player's ELO
    printf("[ELO] Match results:\n");
    for (int i = 0; i < num_players; i++) {
        // Average rating of opponents (exclude this player)
        int avg_opponent = (total_rating - ratings[i]) / (num_players - 1);
        
        int elo_change = calculate_elo_change(ratings[i], avg_opponent, 
                                               placements[i], match_counts[i]);
        int new_rating = ratings[i] + elo_change;
        
        // Ensure rating doesn't go below 0
        if (new_rating < 0) new_rating = 0;

        // Store change for output
        if (out_elo_changes) {
            out_elo_changes[i] = elo_change;
        }
        
        // Update database
        if (db_update_elo(player_ids[i], new_rating) == 0) {
            printf("[ELO]   Player %d: %d -> %d (%s%d)\n", 
                   player_ids[i], ratings[i], new_rating,
                   (elo_change >= 0) ? "+" : "", elo_change);
        }
    }
    
    return 0;
}

// Get tier badge from ELO rating
int get_tier(int elo_rating) {
    if (elo_rating >= 2000) return 3;  // Diamond
    if (elo_rating >= 1500) return 2;  // Gold
    if (elo_rating >= 1000) return 1;  // Silver
    return 0;                           // Bronze
}

// Get tier name as string
const char* get_tier_name(int tier) {
    switch(tier) {
        case 3: return "Diamond";
        case 2: return "Gold";
        case 1: return "Silver";
        default: return "Bronze";
    }
}
