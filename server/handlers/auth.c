#include <stdio.h>
#include <string.h>
#include "../server.h"

void handle_register(int socket_fd, ClientPacket *pkt) {
    ClientInfo *client = find_client_by_socket(socket_fd);
    if (!client) return;
    
    ServerPacket response;
    memset(&response, 0, sizeof(ServerPacket));
    
    response.type = MSG_AUTH_RESPONSE;
    response.code = db_register_user(pkt->username, pkt->email, pkt->password);
    
    if (response.code == AUTH_SUCCESS) {
        // Auto-login logic
        User user;
        if (db_login_user(pkt->username, pkt->password, &user) == AUTH_SUCCESS) {
            client->user_id = user.id;
            strncpy(client->username, user.username, MAX_USERNAME - 1);
            strncpy(client->display_name, user.display_name, MAX_DISPLAY_NAME - 1);
            client->is_authenticated = 1;

            response.payload.auth.user_id = user.id;
            strncpy(response.payload.auth.username, user.username, MAX_USERNAME - 1);
            strncpy(response.payload.auth.display_name, user.display_name, MAX_DISPLAY_NAME - 1);
            response.payload.auth.elo_rating = user.elo_rating;

            char token[64];
            generate_session_token(token, 64);
            if (db_update_session_token(user.id, token) == 0) {
                strncpy(response.payload.auth.session_token, token, 63);
                strncpy(client->session_token, token, 63);
            }

            strcpy(response.message, "Registration successful - welcome!");
            log_event("AUTH", "Registration + Auto-login: %s (ID: %d, ELO: %d)", 
                   user.username, user.id, user.elo_rating);
        } else {
             strcpy(response.message, "Registration successful");
        }
    } else {
        if (response.code == AUTH_USERNAME_EXISTS) strcpy(response.message, "Username is taken. Try another.");
        else if (response.code == AUTH_EMAIL_EXISTS) strcpy(response.message, "Email is taken. Try another.");
        else if (response.code == AUTH_USER_EXISTS) strcpy(response.message, "Email and username are taken. Try another.");
        else if (response.code == AUTH_INVALID) strcpy(response.message, "Invalid registration data");
        else strcpy(response.message, "Registration failed");
    }
    send_response(socket_fd, &response);
}

void handle_login(int socket_fd, ClientPacket *pkt) {
    ClientInfo *client = find_client_by_socket(socket_fd);
    if (!client) return;

    ServerPacket response;
    memset(&response, 0, sizeof(ServerPacket));
    User user;
    
    response.type = MSG_AUTH_RESPONSE;
    response.code = db_login_user(pkt->username, pkt->password, &user);
    
    if (response.code == AUTH_SUCCESS) {
        client->user_id = user.id;
        strncpy(client->username, user.username, MAX_USERNAME - 1);
        strncpy(client->display_name, user.display_name, MAX_DISPLAY_NAME - 1);
        client->is_authenticated = 1;
        
        response.payload.auth.user_id = user.id;
        strncpy(response.payload.auth.username, user.username, MAX_USERNAME - 1);
        strncpy(response.payload.auth.display_name, user.display_name, MAX_DISPLAY_NAME - 1);
        response.payload.auth.elo_rating = user.elo_rating;
        
        char token[64];
        generate_session_token(token, 64);
        if (db_update_session_token(user.id, token) == 0) {
            strncpy(response.payload.auth.session_token, token, 63);
            strncpy(client->session_token, token, 63);
        }
        
        strcpy(response.message, "Login successful");
        log_event("AUTH", "Login: %s (ID: %d, ELO: %d)", user.username, user.id, user.elo_rating);
        
        // SMART REJOIN logic
        for (int i = 0; i < MAX_LOBBIES; i++) {
             Lobby *lb = find_lobby(i);
             if (lb && lb->status == LOBBY_PLAYING) {
                 GameState *gs = &active_games[i];
                 for(int p=0; p < gs->num_players; p++) {
                     if (strcmp(gs->players[p].username, user.username) == 0) {
                         log_event("RECONNECT", "User %s found in active lobby %d", user.username, i);
                         client->lobby_id = i;
                         client->player_id_in_game = p;
                         
                         ServerPacket lobby_pkt;
                         memset(&lobby_pkt, 0, sizeof(ServerPacket));
                         lobby_pkt.type = MSG_LOBBY_UPDATE; 
                         lobby_pkt.code = 0;
                         lobby_pkt.payload.lobby = *lb;
                         send_response(client->socket_fd, &lobby_pkt);
                         broadcast_game_state(i);
                         break;
                     }
                 }
             }
        }
    } else {
        strcpy(response.message, "Invalid credentials");
    }
    send_response(socket_fd, &response);
}

void handle_login_with_token(int socket_fd, ClientPacket *pkt) {
    ClientInfo *client = find_client_by_socket(socket_fd);
    if (!client) return;

    ServerPacket response;
    memset(&response, 0, sizeof(ServerPacket));
    User user;
    
    response.type = MSG_AUTH_RESPONSE;
    if (db_get_user_by_token(pkt->session_token, &user) == 0) {
        response.code = AUTH_SUCCESS;
        client->user_id = user.id;
        strncpy(client->username, user.username, MAX_USERNAME - 1);
        strncpy(client->display_name, user.display_name, MAX_DISPLAY_NAME - 1);
        client->is_authenticated = 1;
        strncpy(client->session_token, user.session_token, 63);
        
        response.payload.auth.user_id = user.id;
        strncpy(response.payload.auth.username, user.username, MAX_USERNAME - 1);
        strncpy(response.payload.auth.display_name, user.display_name, MAX_DISPLAY_NAME - 1);
        response.payload.auth.elo_rating = user.elo_rating;
        strncpy(response.payload.auth.session_token, user.session_token, 63);
        strcpy(response.message, "Auto-login successful");
        
        log_event("AUTH", "Auto-Login: %s (ID: %d)", user.username, user.id);
        
        // SMART REJOIN logic
        for (int i = 0; i < MAX_LOBBIES; i++) {
             Lobby *lb = find_lobby(i);
             if (lb && lb->status == LOBBY_PLAYING) {
                 GameState *gs = &active_games[i];
                 for(int p=0; p < gs->num_players; p++) {
                     if (strcmp(gs->players[p].username, user.username) == 0) {
                         log_event("RECONNECT", "User %s found in active lobby %d", user.username, i);
                         client->lobby_id = i;
                         client->player_id_in_game = p;
                         
                         ServerPacket lobby_pkt;
                         memset(&lobby_pkt, 0, sizeof(ServerPacket));
                         lobby_pkt.type = MSG_LOBBY_UPDATE; 
                         lobby_pkt.code = 0;
                         lobby_pkt.payload.lobby = *lb;
                         send_response(client->socket_fd, &lobby_pkt);
                         broadcast_game_state(i);
                         break;
                     }
                 }
             }
        }
    } else {
        response.code = AUTH_FAIL;
        strcpy(response.message, "Session expired or invalid");
    }
    send_response(socket_fd, &response);
}
