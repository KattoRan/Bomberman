#include <stdio.h>
#include <string.h>
#include "../server.h"

void handle_chat(int socket_fd, ClientPacket *pkt) {
    ClientInfo *client = find_client_by_socket(socket_fd);
    if (!client) return;
    
    if (!client->is_authenticated || client->lobby_id < 0) {
        return;  // Ignore if not authenticated or not in lobby
    }
    
    // Validate message length
    pkt->chat_message[199] = '\0';  // Force null termination
    int msg_len = strlen(pkt->chat_message);
    if (msg_len == 0 || msg_len > 199) {
        return;  // Ignore empty or too long messages
    }
    
    // Find sender's player ID in the lobby
    Lobby* lobby = find_lobby(client->lobby_id);
    if (!lobby) return;
    
    int sender_player_id = -1;
    for (int i = 0; i < lobby->num_players; i++) {
        if (strcmp(lobby->players[i].username, client->username) == 0) {
            sender_player_id = i;
            break;
        }
    }
    
    if (sender_player_id < 0) return;  // Sender not found in lobby
    
    // Store in chat history
    LobbyChat* chat = &lobby_chats[client->lobby_id];
    if (chat->count < MAX_CHAT_HISTORY) {
        ChatHistoryEntry* entry = &chat->messages[chat->count];
        strncpy(entry->sender_username, client->username, MAX_USERNAME - 1);
        entry->sender_username[MAX_USERNAME - 1] = '\0';
        strncpy(entry->message, pkt->chat_message, 199);
        entry->message[199] = '\0';
        entry->timestamp = (uint32_t)time(NULL);
        entry->player_id = sender_player_id;
        chat->count++;
    } else {
        // Shift array and add new message (FIFO)
        for (int i = 0; i < MAX_CHAT_HISTORY - 1; i++) {
            chat->messages[i] = chat->messages[i + 1];
        }
        ChatHistoryEntry* entry = &chat->messages[MAX_CHAT_HISTORY - 1];
        strncpy(entry->sender_username, client->username, MAX_USERNAME - 1);
        entry->sender_username[MAX_USERNAME - 1] = '\0';
        strncpy(entry->message, pkt->chat_message, 199);
        entry->message[199] = '\0';
        entry->timestamp = (uint32_t)time(NULL);
        entry->player_id = sender_player_id;
    }
    
    // Broadcast to all players in the lobby
    ServerPacket chat_msg;
    memset(&chat_msg, 0, sizeof(ServerPacket));
    chat_msg.type = MSG_CHAT;
    chat_msg.code = 0;
    strncpy(chat_msg.payload.chat_msg.sender_username, client->username, MAX_USERNAME - 1);
    chat_msg.payload.chat_msg.sender_username[MAX_USERNAME - 1] = '\0';
    strncpy(chat_msg.payload.chat_msg.message, pkt->chat_message, 199);
    chat_msg.payload.chat_msg.message[199] = '\0';
    chat_msg.payload.chat_msg.timestamp = (uint32_t)time(NULL);
    chat_msg.payload.chat_msg.player_id = sender_player_id;
    
    // Send to all clients in the same lobby
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].lobby_id == client->lobby_id && clients[i].is_authenticated) {
            send_response(clients[i].socket_fd, &chat_msg);
        }
    }
    
    log_event("CHAT", "Lobby %d - %s: %s", client->lobby_id, client->username, pkt->chat_message);
}
