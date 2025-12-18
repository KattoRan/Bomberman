# Requirements Document

## Introduction

This document specifies the requirements for a real-time multiplayer Bomberman game system. The system consists of a C-based server using raw TCP sockets and SQLite3 for persistence, and a client that provides user interface and gameplay experience. The client will be implemented as SDL2-based graphical client. Players can register accounts, manage friends, join competitive matches with ELO ranking, and compete in classic Bomberman-style matches with power-ups and advanced networking features.

## Glossary

- **Server**: The backend C application that manages game state, user authentication, ELO ranking, and network communication using raw TCP sockets
- **Client**: The frontend application that players interact with for interface, input, and networking 
- **Player**: A registered user participating in a game session with persistent statistics and ELO rating
- **Room**: A game lobby where players gather before starting a match, with host privileges and access controls
- **Host**: The player who created a room and has privileges to start games, kick players, and lock rooms
- **Game State**: The current status of an active match including map, player positions, bombs, explosions, and power-ups
- **Game_Arena**: The 15x13 grid-based map where matches take place
- **Bomb**: An explosive device placed by a player that detonates after a 3-second timer
- **Explosion**: The area effect created when a bomb detonates, lasting 0.5 seconds
- **Power_Up**: Collectible items (BombUp, FireUp, SpeedUp) that enhance player abilities
- **ELO_System**: Rating system for competitive player ranking with tier badges
- **Friend_System**: Social system allowing players to send requests, manage friends, and see online status
- **Binary_Protocol**: Custom packet format using packed structs with headers for efficient network communication
- **Client_Prediction**: Local movement prediction with server reconciliation for responsive gameplay
- **Delta_Compression**: Sending only changed game state data to reduce network bandwidth
- **Session_Token**: Authentication token for reconnection and session persistence
- **Replay_System**: Recording and playback system for match analysis using deterministic inputs
- **SDL2_Interface**: Primary graphical user interface with comprehensive visual effects and modern UI components
- **Debug_Output**: Comprehensive logging system for monitoring client-server communication
- **Connection_Info**: Client connection details including IP address, port, and socket descriptor
- **Packet_Logging**: System for logging network packet details and communication flow
- **SDL2_Primary**: SDL2 client as the main implementation

## Requirements

### Requirement 1

**User Story:** As a new player, I want to register an account with username, email, and password, so that I can access the game system and track my progress.

#### Acceptance Criteria

1. WHEN a user submits registration credentials, THE Server SHALL validate the username is unique and create a new user record
2. WHEN a user submits registration data, THE Server SHALL hash the password with salt before storing in SQLite database
3. WHEN registration is successful, THE Server SHALL automatically log the user in and generate a session token
4. WHEN a user attempts registration, THE Server SHALL require username, email, and password fields for completion
5. WHEN registration fails due to duplicate username, THE Server SHALL return an error message indicating the username is taken

### Requirement 2

**User Story:** As a registered player, I want to log in with my credentials, so that I can access my profile and join games.

#### Acceptance Criteria

1. WHEN a user submits valid login credentials, THE Server SHALL authenticate against SQLite database and generate a session token
2. WHEN a user provides login credentials, THE Server SHALL accept either email or username (not display name) as the login identifier
3. WHEN login credentials are invalid, THE Server SHALL return an authentication error
4. WHEN login is successful, THE Server SHALL load user profile data including User ID, display_name, ELO rating, and statistics
5. WHEN login is successful, THE Server SHALL mark the user as online and notify friends of status change (if friends is not playing)


### Requirement 3

**User Story:** As a player, I want to move my character smoothly without getting stuck in walls, so that I can navigate the arena effectively.

#### Acceptance Criteria

1. WHEN a player presses WASD or arrow keys, THE Client SHALL process input and send INPUT_PACKET to Server
2. WHEN the Server receives movement input, THE Server SHALL validate movement against the 15x13 map grid using AABB collision detection
3. WHEN a player hits a wall edge, THE Server SHALL implement corner sliding logic to slide them into the corridor
4. WHEN movement is validated, THE Server SHALL update player position and broadcast the update to all clients within one server tick
5. WHEN a player inputs movement, THE Client SHALL implement client-side prediction for immediate local movement feedback

### Requirement 4

**User Story:** As a player, I want to place bombs that destroy crates and kill enemies, so that I can eliminate opponents and clear paths.

#### Acceptance Criteria

1. WHEN a player places a bomb, THE Server SHALL handle the 3-second timer server-side
2. WHEN a bomb explodes, THE Server SHALL propagate explosion in 4 directions, stopping at hard walls and destroying soft walls
3. WHEN an explosion occurs, THE Server SHALL eliminate any players in the blast area
4. WHEN an explosion occurs, THE Server SHALL set explosion duration to 0.5 seconds
5. THE Server SHALL broadcast EXPLOSION_PACKET containing all affected tiles to all clients

### Requirement 5

**User Story:** As a player, I want varied map layouts every game, so that matches feel fresh and strategic.

#### Acceptance Criteria

1. WHEN a game starts, THE Server SHALL generate a fixed 15x13 grid map
2. THE Server SHALL place hard walls in a fixed pattern at every even tile coordinate
3. THE Server SHALL randomly generate soft wall positions using a server-side seed
4. WHEN a soft wall is destroyed, THE Server SHALL update map state and notify all clients
5. THE Server SHALL ensure player spawn areas in corners are clear of walls

### Requirement 6

**User Story:** As a player, I want clear match results and win conditions, so that I understand game outcomes.

#### Acceptance Criteria

1. WHEN only one player survives, THE Server SHALL declare them the winner (Classic Mode)
2. WHEN the last two players die simultaneously, THE Server SHALL declare a draw
3. WHEN the match timer reaches 3 minutes, THE Server SHALL activate sudden death mode with walls shrinking from outside in
4. WHEN a match ends, THE Server SHALL calculate ELO rating changes for all participants
5. WHEN a match ends, THE Server SHALL return players to the lobby and display match results

### Requirement 7

**User Story:** As a player, I want to collect power-ups to become stronger, so that I can gain strategic advantages.

#### Acceptance Criteria

1. WHEN a soft wall is destroyed, THE Server SHALL have a 20% chance to spawn a power-up item
2. THE Server SHALL support BombUp, FireUp, and SpeedUp power-up types with respective enhancements
3. WHEN a player moves over a power-up, THE Server SHALL apply the enhancement immediately
4. THE Server SHALL enforce maximum caps for speed and range values according to system constants
5. THE Server SHALL persist power-up effects for the duration of the match

### Requirement 8

**User Story:** As a developer, I want a custom binary protocol for efficient network communication, so that the game performs well over the network.

#### Acceptance Criteria

1. THE Server and Client SHALL use packed struct format with standard Header containing PacketType and PayloadSize fields
2. THE protocol SHALL use bit packing for boolean flags to save bandwidth
3. THE Server SHALL implement consistent byte ordering for network communication
4. THE network layer SHALL handle packet fragmentation using a ring buffer to accumulate incoming bytes
5. THE system SHALL only process packets when the buffer contains the full PayloadSize specified in the header

### Requirement 9

**User Story:** As a player, I want responsive gameplay despite network latency, so that the game feels smooth and immediate.

#### Acceptance Criteria

1. WHEN a player inputs movement, THE Client SHALL immediately update the local player position
2. THE Client SHALL store input history in a circular buffer with sequence numbers
3. WHEN the Client receives server position updates, THE Client SHALL reconcile predicted position with authoritative server position
4. WHEN position discrepancy exceeds threshold, THE Client SHALL snap to server position and replay pending inputs
5. THE Server SHALL send delta state compression by transmitting only entities that changed since the last tick

### Requirement 10

**User Story:** As a player, I want automatic reconnection if my connection drops, so that I don't lose my progress in matches.

#### Acceptance Criteria

1. WHEN TCP connection drops, THE Client SHALL attempt to reconnect for 10 seconds
2. WHEN reconnecting, THE Client SHALL send a SESSION_TOKEN to restore session
3. THE Server SHALL restore the player to their current entity in the active match instead of eliminating them
4. THE Server SHALL handle sticky packets and partial packets correctly during network communication
5. THE network layer SHALL implement fault tolerance for connection interruptions

### Requirement 11

**User Story:** As a system administrator, I want high-performance server architecture, so that the game can handle multiple concurrent players smoothly.

#### Acceptance Criteria

1. THE Server SHALL implement multithreaded architecture with Main Thread running game logic at 60Hz tick rate
2. THE Server SHALL use a dedicated Network Thread handling epoll/IOCP and pushing packets to thread-safe queue
3. THE Server SHALL use a Database Thread for asynchronous SQLite writes to prevent blocking the game loop
4. THE Server SHALL pre-allocate memory using custom memory arena with linear allocator for game entities
5. THE Server SHALL avoid malloc/free calls inside the main game loop for consistent performance

### Requirement 12

**User Story:** As a developer, I want deterministic replay capability, so that matches can be analyzed and debugged.

#### Acceptance Criteria

1. THE Server SHALL record the random seed and all player inputs (tick + key) to a binary file
2. THE Server SHALL implement replay mode that can load the file and simulate the exact match without network activity
3. THE replay system SHALL ensure deterministic behavior for match analysis
4. THE Server SHALL store replay files with match metadata including player names and final results
5. THE replay system SHALL support playback at different speeds for analysis

### Requirement 13

**User Story:** As a player, I want persistent data storage, so that my progress and relationships are maintained across sessions.

#### Acceptance Criteria

1. THE Server SHALL use SQLite database with Users table containing at least: id (Primary Key), username (Unique, Immutable), and display_name (Mutable)
2. THE Friendships table SHALL store user_id_1, user_id_2, and status (PENDING, ACCEPTED)
3. THE Server SHALL hash and salt all passwords before database storage
4. THE Server SHALL track match history, player statistics, and ELO ratings persistently linked to User ID
5. THE Database Thread SHALL handle all SQLite operations asynchronously to maintain game performance

### Requirement 14

**User Story:** As a player, I want clear graphical game display with modern visual effects, so that I can understand the game state and interact effectively.

#### Acceptance Criteria

1. THE SDL2 Client SHALL load textures and fonts, implement particle systems, and provide visual effects
2. THE SDL2 Client SHALL render the game map with enhanced tile graphics and animations
3. THE SDL2 Client SHALL display player sprites with distinct colors and smooth movement animations
4. THE SDL2 Client SHALL implement HUD overlay with player status, bomb count, power-ups, and timer
5. THE SDL2 Client SHALL update the display in real-time as game state changes with 60 FPS performance

### Requirement 15

**User Story:** As a player, I want comprehensive friend management, so that I can build and maintain my social circle within the game.

#### Acceptance Criteria

1. WHEN a player sends a friend request, THE Server SHALL allow entering a display name to send FRIEND_REQUEST
2. WHEN a user receives a friend request, THE Server SHALL show immediate notification if online, or display on next login if offline
3. THE Server SHALL allow users to accept or decline pending requests from a dedicated interface
4. WHEN a player deletes a friend, THE Server SHALL update the database and notify the other user immediately if online
5. THE Client SHALL display a friends sidebar with real-time status indicators (Online-Green, In-Game-Blue, Offline-Gray)

### Requirement 16

**User Story:** As a competitive player, I want an advanced ELO ranking system, so that I can track my skill progression and compete at appropriate levels.

#### Acceptance Criteria

1. THE Server SHALL implement standard ELO rating algorithm with base rating of 1200
2. THE Server SHALL adjust K-factor based on matches played (high K for new players, low K for veterans)
3. THE Server SHALL implement zero-sum point system where points gained by winner equal points lost by losers
4. THE Server SHALL display tier badges (Bronze: 0-999, Silver: 1000-1499, Gold: 1500-1999, Diamond: 2000+)
5. THE Client SHALL show player tier badges next to names in lobby interface

### Requirement 17

**User Story:** As a player, I want detailed statistics tracking, so that I can analyze my performance over time.

#### Acceptance Criteria

1. THE Server SHALL track lifetime statistics including Total_Matches, Wins, Win_Rate, Total_Kills, K/D_Ratio, Most_Used_Powerup
2. THE Client SHALL provide PROFILE_REQUEST to fetch statistics data for display in Career tab
3. THE Server SHALL update statistics after each match completion
4. THE Client SHALL display statistics in an organized profile interface with charts and progress indicators
5. THE Server SHALL maintain historical match data for trend analysis

### Requirement 18

**User Story:** As a player, I want skill-based matchmaking, so that I can quickly find matches with players of similar skill level.

#### Acceptance Criteria

1. WHEN a player clicks Quick Play, THE Client SHALL send FIND_MATCH_REQUEST to server
2. THE Server SHALL scan open lobbies to find rooms where average ELO is within ±200 of player's ELO
3. WHEN no matching room is found within 5 seconds, THE Server SHALL expand search range or create new public room
4. THE Server SHALL automatically place player in appropriate skill-matched room
5. THE matchmaking system SHALL prioritize connection quality and ping for local network players

### Requirement 19

**User Story:** As a player, I want leaderboards to see top players, so that I can track my ranking and compete for top positions.

#### Acceptance Criteria

1. THE Server SHALL maintain global top 100 ranking accessible via GET_TOP_100 request
2. THE Client SHALL display leaderboards with player names, ELO ratings, and tier badges
3. THE Server SHALL provide friends-only leaderboard filtering to show ranking among friends
4. THE leaderboard SHALL update in real-time as ELO ratings change after matches
5. THE Client SHALL highlight the current player's position in leaderboard displays

### Requirement 20

**User Story:** As a player, I want private rooms with access codes, so that I can play matches with specific people.

#### Acceptance Criteria

1. WHEN creating a room, THE Host SHALL be able to toggle private mode and set access code
2. THE Server SHALL generate random 4-6 digit codes or allow host to set custom password
3. THE Client SHALL provide "Join by Code" input field in lobby interface
4. THE Server SHALL reject join requests for private rooms without correct access code
5. THE private room system SHALL work seamlessly with friend invitations

### Requirement 21

**User Story:** As a room host, I want full control over my room, so that I can manage the game experience.

#### Acceptance Criteria

1. THE Host SHALL be able to kick players using KICK_PACKET with target player ID
2. WHEN a player is kicked, THE Server SHALL remove them and send notification "You have been kicked"
3. THE Host SHALL be able to lock rooms to prevent new players from joining
4. THE Server SHALL reject all connection attempts to locked rooms regardless of access codes
5. THE Host SHALL retain all privileges until they leave or transfer host status

### Requirement 22

**User Story:** As a player, I want spectator mode for ongoing matches, so that I can watch games and learn from other players.

#### Acceptance Criteria

1. WHEN joining an in-progress room, THE Server SHALL automatically set player as spectator
2. THE Server SHALL send map updates to spectators but reject any input packets from them
3. THE Client SHALL display spectator interface with different UI indicating spectator status
4. THE spectator system SHALL allow seamless transition to player when match ends
5. THE Server SHALL limit spectator count to prevent performance issues

### Requirement 23

**User Story:** As a player, I want chat and communication features, so that I can interact with other players.

#### Acceptance Criteria

1. THE Client SHALL provide lobby chat interface for text communication
2. THE Server SHALL broadcast chat messages to all players in the same room
3. THE Client SHALL implement emote system for quick communication during matches
4. THE chat system SHALL include basic moderation features like message length limits
5. THE Server SHALL log chat messages for moderation purposes while respecting user privacy

### Requirement 24

**User Story:** As a player, I want an intuitive graphical menu interface, so that I can easily navigate to different game features.

#### Acceptance Criteria

1. THE SDL2 Client SHALL display a graphical main menu with buttons for Login, Register, Quick Play, Browse Rooms, Friends, Profile, Settings, and Exit
2. THE SDL2 Client SHALL use consistent visual styling with modern UI components and smooth animations
3. THE SDL2 Client SHALL implement mouse and keyboard input handling for menu navigation
4. THE SDL2 Client SHALL provide visual feedback for button hover states and selected menu items
5. THE SDL2 Client SHALL display the game title, version number, and connection status with graphical indicators

### Requirement 25

**User Story:** As a new user, I want clear graphical registration and login forms, so that I can easily create an account and access the game.

#### Acceptance Criteria

1. THE SDL2 Client SHALL display graphical registration form with input fields for username, email, password, and confirm password
2. THE SDL2 Client SHALL implement text input handling with cursor positioning and password masking
3. THE SDL2 Client SHALL provide real-time validation feedback with visual indicators for invalid inputs
4. THE SDL2 Client SHALL display password strength indicators and field validation status
5. THE SDL2 Client SHALL show loading animations during authentication requests and clear success/error dialogs

### Requirement 26

**User Story:** As a player, I want a comprehensive graphical lobby interface, so that I can browse rooms, manage friends, and join matches easily.

#### Acceptance Criteria

1. THE SDL2 Client SHALL display a tabbed interface with sections for Room List, Friends, Profile, and Leaderboards
2. THE SDL2 Client SHALL show room list with graphical cards displaying Room Name, Players, ELO Range, and Status with click selection
3. THE SDL2 Client SHALL implement scrollable lists with smooth scrolling and pagination for large datasets
4. THE SDL2 Client SHALL provide search bars and filter dropdowns for rooms by name, player count, and ELO range
5. THE SDL2 Client SHALL display real-time updates with smooth animations when rooms are created, destroyed, or player counts change

### Requirement 27

**User Story:** As a player, I want an organized graphical friends management interface, so that I can easily manage my social connections.

#### Acceptance Criteria

1. THE SDL2 Client SHALL display friends list with graphical status indicators, ELO ratings, and current activity icons
2. THE SDL2 Client SHALL provide "Add Friend" dialog with search field and graphical request sending interface
3. THE SDL2 Client SHALL show pending friend requests with styled Accept/Decline buttons and notification badges
4. THE SDL2 Client SHALL implement right-click context menus for friends (View Profile, Invite to Room, Remove Friend)
5. THE SDL2 Client SHALL display friend activity feed with graphical cards showing recent matches and achievements

### Requirement 28

**User Story:** As a player, I want detailed graphical profile and statistics screens, so that I can track my progress and achievements.

#### Acceptance Criteria

1. THE SDL2 Client SHALL display player profile with avatar, username, ELO rating, tier badge, and rank with graphical elements
2. THE SDL2 Client SHALL show detailed statistics including matches played, win rate, K/D ratio, favorite power-ups with visual charts
3. THE SDL2 Client SHALL implement graphical charts and graphs for visualizing performance trends over time
4. THE SDL2 Client SHALL display match history with expandable cards for each game with detailed statistics
5. THE SDL2 Client SHALL provide achievement system with progress bars and animated unlock notifications

### Requirement 29

**User Story:** As a room host, I want comprehensive room management controls, so that I can configure and manage my game sessions.

#### Acceptance Criteria

1. THE Client SHALL display room creation dialog with options for room name, privacy settings, access code, and player limit
2. THE Client SHALL show room lobby interface with player list, ready status indicators, and host controls
3. THE Client SHALL provide host-only controls including kick buttons, room lock toggle, and start game button
4. THE Client SHALL implement chat interface with text input, message history, and emote buttons
5. THE Client SHALL display room settings panel with map selection and game mode options

### Requirement 30

**User Story:** As a player, I want responsive graphical game interface, so that I can monitor my status and game progress during matches.

#### Acceptance Criteria

1. THE SDL2 Client SHALL display HUD with graphical indicators for player health, bomb count, power-up status, and match timer
2. THE SDL2 Client SHALL render the game map with enhanced tile graphics, player sprites, and visual effects
3. THE SDL2 Client SHALL implement animated notifications for damage, power-up collection, and elimination messages
4. THE SDL2 Client SHALL display match results screen with winner announcement, detailed statistics, and ELO changes
5. THE SDL2 Client SHALL provide pause menu with graphical buttons to resume, view controls, or forfeit match

### Requirement 31

**User Story:** As a player, I want customizable settings and preferences, so that I can tailor the game experience to my needs.

#### Acceptance Criteria

1. THE Client SHALL provide settings menu with tabs for Controls, Gameplay, and Account
2. THE Client SHALL allow key binding customization with conflict detection and reset to defaults
3. THE Client SHALL save all settings to configuration file and apply changes immediately
4. THE Server SHALL allow users to update their display_name via the Account settings
5. THE Server SHALL reject any attempts to modify the username field once registered, ensuring it remains the unique immutable identifier

### Requirement 32

**User Story:** As a player, I want clear graphical feedback and status updates, so that the game feels responsive and informative.

#### Acceptance Criteria

1. THE SDL2 Client SHALL implement smooth screen transitions between menu screens with animated loading indicators
2. THE SDL2 Client SHALL provide immediate visual feedback for user input including button animations and status updates
3. THE SDL2 Client SHALL display notification system with toast messages for friend requests, invitations, and achievements
4. THE SDL2 Client SHALL implement visual status indicators for menu selections and game state changes
5. THE SDL2 Client SHALL use consistent visual styling and layout throughout all interface elements

### Requirement 33

**User Story:** As a player, I want accessibility features, so that the game is usable regardless of my abilities or preferences.

#### Acceptance Criteria

1. THE SDL2 Client SHALL support keyboard-only navigation with clear visual focus indicators and tab order
2. THE SDL2 Client SHALL provide high contrast display options and colorblind-friendly color schemes
3. THE SDL2 Client SHALL implement scalable UI elements that work across different screen resolutions
4. THE SDL2 Client SHALL provide tooltips and visual descriptions for all game elements and status information
5. THE SDL2 Client SHALL include visual feedback for all important game interactions and events

### Requirement 34

**User Story:** As a player, I want error handling and user feedback, so that I understand what's happening when issues occur.

#### Acceptance Criteria

1. THE Client SHALL display clear error messages for network issues, authentication failures, and game errors
2. THE Client SHALL implement loading screens with progress indicators for long operations
3. THE Client SHALL provide confirmation dialogs for destructive actions like deleting friends or leaving matches
4. THE Client SHALL show connection status indicators and reconnection attempts
5. THE Client SHALL implement graceful degradation when features are unavailable due to network issues

### Requirement 35

**User Story:** As a developer, I want modular SDL2 interface architecture, so that the interface is maintainable and extensible.

#### Acceptance Criteria

1. THE SDL2 Client SHALL implement UI component system with reusable elements for buttons, input fields, and panels
2. THE SDL2 Client SHALL use state management system for tracking interface state and smooth screen transitions
3. THE SDL2 Client SHALL implement event-driven interface updates that respond to game state changes
4. THE SDL2 Client SHALL provide consistent styling system with themes across all interface elements
5. THE SDL2 Client SHALL structure interface code with clear separation between rendering, input handling, and business logic

### Requirement 36

**User Story:** As a developer, I want comprehensive console logging and debugging output, so that I can monitor client-server communication and troubleshoot issues effectively.

#### Acceptance Criteria

1. WHEN a client connects to the server, THE Server SHALL print client connection details including IP address, port number, and socket descriptor to console
2. WHEN a client sends data to the server, THE Server SHALL log the packet type, size, and source client information to console
3. WHEN the server sends data to clients, THE Server SHALL print the destination client details and packet information to console
4. WHEN network events occur, THE Client SHALL display connection status, server responses, and error messages to console
5. THE Server SHALL provide detailed console output for all client lifecycle events including connect, disconnect, and timeout scenarios

### Requirement 37

**User Story:** As a developer, I want SDL2-focused development approach, so that I can deliver a modern graphical experience.

#### Acceptance Criteria

1. THE SDL2 Client SHALL be the primary implementation providing all core functionality including authentication, lobbies, and gameplay
2. THE SDL2 Client SHALL provide comprehensive graphical interface with modern UI components and visual effects
4. THE SDL2 Client SHALL serve as the main user-facing application with production-ready features

### Requirement 38

**User Story:** As a developer, I want a simple build system, so that the game can be easily compiled and maintained.

#### Acceptance Criteria

1. THE build system SHALL use a standard Makefile for compilation
2. THE Server SHALL use standard C libraries and networking APIs
3. THE Client SHALL use SDL2 libraries for graphics
4. THE build system SHALL provide clear compilation targets and dependency management
5. THE code SHALL be written in portable C with minimal platform-specific code

### Requirement 38

**User Story:** As a developer, I want context-aware lobby logging similar to table-based logging, so that I can visualize the full state of all participants whenever a game event occurs for easier debugging.

#### Acceptance Criteria

1. THE build system SHALL use a standard Makefile for compilation
2. THE Server SHALL use standard C libraries and networking APIs
3. THE Client SHALL use SDL2 libraries for graphics
4. THE build system SHALL provide clear compilation targets and dependency management
5. THE code SHALL be written in portable C with minimal platform-specific code