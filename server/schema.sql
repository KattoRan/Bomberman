-- Bomberman SQLite3 Database Schema
-- Phase 1: Core tables for authentication, friends, ELO, and statistics

-- Users table: Core authentication and profile data
CREATE TABLE IF NOT EXISTS Users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,           -- Immutable unique identifier
    display_name TEXT NOT NULL,              -- Mutable display name
    email TEXT UNIQUE NOT NULL,              -- For login and verification
    password_hash TEXT NOT NULL,             -- Hashed password
    salt TEXT NOT NULL,                      -- Salt for password hashing
    elo_rating INTEGER DEFAULT 1200,         -- ELO ranking (start at 1200)
    session_token TEXT,                      -- For persistent login
    session_expiry TIMESTAMP,                -- Token expiration
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Create index on email for fast login lookup
CREATE INDEX IF NOT EXISTS idx_users_email ON Users(email);
CREATE INDEX IF NOT EXISTS idx_users_username ON Users(username);

-- Friendships table: Friend relationships and pending requests
CREATE TABLE IF NOT EXISTS Friendships (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id_1 INTEGER NOT NULL,              -- First user ID
    user_id_2 INTEGER NOT NULL,              -- Second user ID
    status TEXT NOT NULL CHECK(status IN ('PENDING', 'ACCEPTED')),
    requested_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    accepted_at TIMESTAMP,
    FOREIGN KEY (user_id_1) REFERENCES Users(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id_2) REFERENCES Users(id) ON DELETE CASCADE,
    -- Ensure no duplicate friendships
    UNIQUE(user_id_1, user_id_2)
);

-- Create indexes for fast friend lookups
CREATE INDEX IF NOT EXISTS idx_friendships_user1 ON Friendships(user_id_1);
CREATE INDEX IF NOT EXISTS idx_friendships_user2 ON Friendships(user_id_2);
CREATE INDEX IF NOT EXISTS idx_friendships_status ON Friendships(status);

-- Statistics table: Player performance metrics
CREATE TABLE IF NOT EXISTS Statistics (
    user_id INTEGER PRIMARY KEY,
    total_matches INTEGER DEFAULT 0,
    wins INTEGER DEFAULT 0,
    total_kills INTEGER DEFAULT 0,
    deaths INTEGER DEFAULT 0,
    bombs_planted INTEGER DEFAULT 0,
    walls_destroyed INTEGER DEFAULT 0,
    FOREIGN KEY (user_id) REFERENCES Users(id) ON DELETE CASCADE
);

-- MatchHistory table: Record of completed matches
CREATE TABLE IF NOT EXISTS MatchHistory (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    match_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    winner_id INTEGER,                       -- NULL for draw
    duration_seconds INTEGER,
    num_players INTEGER,
    FOREIGN KEY (winner_id) REFERENCES Users(id) ON DELETE SET NULL
);

CREATE INDEX IF NOT EXISTS idx_match_date ON MatchHistory(match_date);

-- MatchPlayers table: Individual player performance in matches
CREATE TABLE IF NOT EXISTS MatchPlayers (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    match_id INTEGER NOT NULL,
    user_id INTEGER NOT NULL,
    placement INTEGER,                       -- 1st, 2nd, 3rd, 4th
    kills INTEGER DEFAULT 0,
    deaths INTEGER DEFAULT 1,                -- Always 1 or 0 (survived)
    elo_change INTEGER,                      -- ELO points gained/lost
    FOREIGN KEY (match_id) REFERENCES MatchHistory(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES Users(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_match_players_match ON MatchPlayers(match_id);
CREATE INDEX IF NOT EXISTS idx_match_players_user ON MatchPlayers(user_id);
