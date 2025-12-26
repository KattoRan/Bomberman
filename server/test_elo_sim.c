#include <stdio.h>
#include <math.h>

// Copied from server/elo_system.c

int get_k_factor(int matches_played) {
    if (matches_played < 10) return 40;
    if (matches_played < 30) return 30;
    return 20;
}

double calculate_expected_score(int rating_a, int rating_b) {
    return 1.0 / (1.0 + pow(10.0, (rating_b - rating_a) / 400.0));
}

int calculate_elo_change(int player_rating, int avg_opponent_rating, 
                         int placement, int matches_played) {
    double expected = calculate_expected_score(player_rating, avg_opponent_rating);
    double actual = (placement == 1) ? 1.0 : 0.0;
    
    int k = get_k_factor(matches_played);
    int change = (int)(k * (actual - expected));
    
    return change;
}

void simulate_match(int *ratings, int *matches, int num_players) {
    printf("--- Pairwise Match Simulation ---\n");
    printf("Initial Ratings: ");
    for(int i=0; i<num_players; i++) printf("P%d: %d ", i+1, ratings[i]);
    printf("\n");

    // Rank 1 is P1 (index 0), Rank 2 is P2 (index 1), etc.
    // Lower index = better placement here for simulation simplicity.
    int placements[4] = {1, 2, 3, 4}; 

    int total_change_system = 0;

    for (int i = 0; i < num_players; i++) {
        double total_expected = 0;
        double total_actual = 0;
        
        for (int j = 0; j < num_players; j++) {
            if (i == j) continue;
            
            // Calculate expectation for i vs j
            double expected = calculate_expected_score(ratings[i], ratings[j]);
            total_expected += expected;
            
            // Calculate actual result (1.0 if i beat j, 0.0 if i lost to j)
            // Since our array is sorted by rank (index 0 is rank 1), i < j means i beat j
            double actual = (placements[i] < placements[j]) ? 1.0 : 0.0;
            total_actual += actual;
        }
        
        int k = get_k_factor(matches[i]);
        
        // The change is Sum(K * (Actual - Expected)) which is equivalent to K * Sum(Actual - Expected)
        int change = (int)(k * (total_actual - total_expected));
        
        total_change_system += change;
        printf("P%d (Place %d): Rating %d -> Score %.2f/%.2f -> Change: %d\n", 
               i+1, placements[i], ratings[i], total_actual, total_expected, change);
    }
    
    printf("Total System Point Change: %d\n", total_change_system);
}

int main() {
    // Case 1: 4 Players, Equal Rating, New Players (High K)
    int r1[] = {1000, 1000, 1000, 1000};
    int m1[] = {0, 0, 0, 0};
    simulate_match(r1, m1, 4);

    // Case 2: 4 Players, Equal Rating, Experienced (Low K)
    int r2[] = {1500, 1500, 1500, 1500};
    int m2[] = {100, 100, 100, 100};
    simulate_match(r2, m2, 4);

    return 0;
}
