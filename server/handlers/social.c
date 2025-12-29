#include <stdio.h>
#include <string.h>
#include "../server.h"

void handle_friend_request(int socket_fd, ClientPacket *pkt) {
    ClientInfo *client = find_client_by_socket(socket_fd);
    if (!client || !client->is_authenticated) return;
    
    int result = friend_send_request(client->user_id, pkt->target_display_name);
    
    ServerPacket response;
    memset(&response, 0, sizeof(ServerPacket));
    response.type = MSG_NOTIFICATION;
    response.code = result;
    
    if (result == 0) {
        snprintf(response.message, sizeof(response.message), 
                 "Friend request sent to %s", pkt->target_display_name);
    } else if (result == 1) {
        strcpy(response.message, "Already friends");
    } else if (result == 2) {
        strcpy(response.message, "Request already pending");
    } else {
        strcpy(response.message, "Friend request failed");
    }
    
    send_response(socket_fd, &response);
}

void handle_friend_accept(int socket_fd, ClientPacket *pkt) {
    ClientInfo *client = find_client_by_socket(socket_fd);
    if (!client || !client->is_authenticated) return;
    
    int result = friend_accept_request(client->user_id, pkt->target_user_id);
    
    ServerPacket response;
    memset(&response, 0, sizeof(ServerPacket));
    response.type = MSG_NOTIFICATION;
    response.code = result;
    
    if (result == 0) strcpy(response.message, "Friend request accepted");
    else strcpy(response.message, "Failed to accept request");
    
    send_response(socket_fd, &response);
    
    // Refresh friend list if successful
    if (result == 0) {
        // Reuse handle_friend_list? Or manually send response. 
        // Let's manually send to be safe and explicit.
        ServerPacket list_resp;
        memset(&list_resp, 0, sizeof(ServerPacket));
        list_resp.type = MSG_FRIEND_LIST_RESPONSE;
        
        int friend_count = friend_get_list(client->user_id, list_resp.payload.friend_list.friends, 50);
        list_resp.payload.friend_list.count = friend_count;
        
        FriendInfo pending[50];
        int pending_count = friend_get_pending_requests(client->user_id, pending, 50);
        
        FriendInfo sent[50];
        int sent_count = friend_get_sent_requests(client->user_id, sent, 50);

        // Combine logic similar to main.c
        if (friend_count + pending_count + sent_count <= 50) {
            memcpy(&list_resp.payload.friend_list.friends[friend_count], 
                   pending, sizeof(FriendInfo) * pending_count);
            memcpy(&list_resp.payload.friend_list.friends[friend_count + pending_count],
                   sent, sizeof(FriendInfo) * sent_count);
            list_resp.payload.friend_list.count += pending_count + sent_count;
        }
        
        list_resp.code = pending_count | (sent_count << 8);
        send_response(socket_fd, &list_resp);
    }
}

void handle_friend_reject(int socket_fd, ClientPacket *pkt) {
    ClientInfo *client = find_client_by_socket(socket_fd);
    if (!client || !client->is_authenticated) return;

    int result = friend_decline_request(client->user_id, pkt->target_user_id);
    
    ServerPacket response;
    memset(&response, 0, sizeof(ServerPacket));
    response.type = MSG_NOTIFICATION;
    response.code = result;
    
    if (result == 0) strcpy(response.message, "Friend request declined");
    else strcpy(response.message, "Failed to decline request");
    
    send_response(socket_fd, &response);
    
    // Refresh list logic is usually good here too
    if (result == 0) {
        // Reuse logic? Or copy-paste. Copy-paste for now to keep it self-contained.
        ServerPacket list_resp;
        memset(&list_resp, 0, sizeof(ServerPacket));
        list_resp.type = MSG_FRIEND_LIST_RESPONSE;
         // ... (same refresh logic)
        int friend_count = friend_get_list(client->user_id, list_resp.payload.friend_list.friends, 50);
        list_resp.payload.friend_list.count = friend_count;
        
        FriendInfo pending[50];
        int pending_count = friend_get_pending_requests(client->user_id, pending, 50);
        
        FriendInfo sent[50];
        int sent_count = friend_get_sent_requests(client->user_id, sent, 50);
        
        if (friend_count + pending_count + sent_count <= 50) {
            memcpy(&list_resp.payload.friend_list.friends[friend_count], 
                   pending, sizeof(FriendInfo) * pending_count);
             memcpy(&list_resp.payload.friend_list.friends[friend_count + pending_count],
                   sent, sizeof(FriendInfo) * sent_count);
            list_resp.payload.friend_list.count += pending_count + sent_count;
        }
        list_resp.code = pending_count | (sent_count << 8);
        send_response(socket_fd, &list_resp);
    }
}


void handle_friend_remove(int socket_fd, ClientPacket *pkt) {
    ClientInfo *client = find_client_by_socket(socket_fd);
    if (!client || !client->is_authenticated) return;

    int result = friend_remove(client->user_id, pkt->target_user_id);
    
    ServerPacket response;
    memset(&response, 0, sizeof(ServerPacket));
    response.type = MSG_NOTIFICATION;
    response.code = result;
    
    if (result == 0) strcpy(response.message, "Friend removed");
    else strcpy(response.message, "Failed to remove friend");
    
    send_response(socket_fd, &response);
    
    // Refresh friend list
    if (result == 0) {
        ServerPacket list_resp;
        memset(&list_resp, 0, sizeof(ServerPacket));
        list_resp.type = MSG_FRIEND_LIST_RESPONSE;
        
        int friend_count = friend_get_list(client->user_id, list_resp.payload.friend_list.friends, 50);
        list_resp.payload.friend_list.count = friend_count;
        
        FriendInfo pending[50];
        int pending_count = friend_get_pending_requests(client->user_id, pending, 50);
        
        FriendInfo sent[50];
        int sent_count = friend_get_sent_requests(client->user_id, sent, 50);
        
        if (friend_count + pending_count + sent_count <= 50) {
            memcpy(&list_resp.payload.friend_list.friends[friend_count], 
                   pending, sizeof(FriendInfo) * pending_count);
             memcpy(&list_resp.payload.friend_list.friends[friend_count + pending_count],
                   sent, sizeof(FriendInfo) * sent_count);
            list_resp.payload.friend_list.count += pending_count + sent_count;
        }
        list_resp.code = pending_count | (sent_count << 8);
        send_response(socket_fd, &list_resp);
    }
}

void handle_friend_list(int socket_fd, ClientPacket *pkt) {
    (void)pkt;
    ClientInfo *client = find_client_by_socket(socket_fd);
    if (!client || !client->is_authenticated) return;

    ServerPacket response;
    memset(&response, 0, sizeof(ServerPacket));
    response.type = MSG_FRIEND_LIST_RESPONSE;
    
    int friend_count = friend_get_list(client->user_id, response.payload.friend_list.friends, 50);
    response.payload.friend_list.count = friend_count;
    
    FriendInfo pending[50];
    int pending_count = friend_get_pending_requests(client->user_id, pending, 50);
    
    FriendInfo sent[50];
    int sent_count = friend_get_sent_requests(client->user_id, sent, 50);

    if (friend_count + pending_count + sent_count <= 50) {
        memcpy(&response.payload.friend_list.friends[friend_count], 
               pending, sizeof(FriendInfo) * pending_count);
        memcpy(&response.payload.friend_list.friends[friend_count + pending_count],
               sent, sizeof(FriendInfo) * sent_count);
        response.payload.friend_list.count += pending_count + sent_count;
    }
    
    response.code = pending_count | (sent_count << 8);
    
    // Check online status logic - using display_name now
    for (int i = 0; i < response.payload.friend_list.count; i++) {
        // Only check online status for confirmed friends (first friend_count items)
        if (i < friend_count) {
             response.payload.friend_list.friends[i].is_online = 0;
             for (int c = 0; c < num_clients; c++) {
                 if (clients[c].is_authenticated && 
                     strcmp(clients[c].display_name, response.payload.friend_list.friends[i].display_name) == 0) {
                     response.payload.friend_list.friends[i].is_online = 1;
                     if (clients[c].lobby_id != -1) {
                          response.payload.friend_list.friends[i].is_online = 2; // In game/lobby
                     }
                     break;
                 }
             }
        }
    }
    
    send_response(socket_fd, &response);
}

void handle_get_profile(int socket_fd, ClientPacket *pkt) {
    ClientInfo *client = find_client_by_socket(socket_fd);
    if (!client || !client->is_authenticated) return;
    
    ServerPacket response;
    memset(&response, 0, sizeof(ServerPacket));
    response.type = MSG_PROFILE_RESPONSE;
    
    int target_id = client->user_id;
    if (pkt->target_user_id > 0) {
        target_id = pkt->target_user_id;
    } else if (pkt->data > 0) {
        target_id = pkt->data;
    }
    
    if (stats_get_profile(target_id, &response.payload.profile) == 0) {
        response.code = 0; // Success
    } else {
        response.code = 1; // Failed
        strcpy(response.message, "Could not load profile");
    }
    send_response(socket_fd, &response);
}

void handle_get_leaderboard(int socket_fd, ClientPacket *pkt) {
    (void)pkt;
    
    ServerPacket response;
    memset(&response, 0, sizeof(ServerPacket));
    response.type = MSG_LEADERBOARD_RESPONSE;
    response.payload.leaderboard.count = 
        stats_get_leaderboard(response.payload.leaderboard.entries, 100);
    send_response(socket_fd, &response);
}

void handle_invite(int socket_fd, ClientPacket *pkt) {
    ClientInfo *client = find_client_by_socket(socket_fd);
    if (!client || !client->is_authenticated) return;
    
    if (client->lobby_id == -1) {
        ServerPacket response;
        memset(&response, 0, sizeof(ServerPacket));
        response.type = MSG_NOTIFICATION;
        strcpy(response.message, "You must be in a lobby to invite friends");
        send_response(socket_fd, &response);
        return;
    }
    
    ServerPacket response;
    memset(&response, 0, sizeof(ServerPacket));
    
    int target_socket = -1;
    
    // Priority: Lookup by ID
    if (pkt->target_user_id > 0) {
        for (int i = 0; i < num_clients; i++) {
            if (clients[i].is_authenticated && clients[i].user_id == pkt->target_user_id) {
                target_socket = clients[i].socket_fd;
                break;
            }
        }
    }
    
    // Fallback: Lookup by display name (for backward compatibility or if ID missing)
    if (target_socket == -1) {
        for (int i = 0; i < num_clients; i++) {
            if (clients[i].is_authenticated && 
                strcmp(clients[i].display_name, pkt->target_display_name) == 0) {
                target_socket = clients[i].socket_fd;
                break;
            }
        }
    }
    
    if (target_socket != -1) {
        Lobby *lobby = find_lobby(client->lobby_id);
        if (lobby) {
            ServerPacket invite_pkt;
            memset(&invite_pkt, 0, sizeof(ServerPacket));
            invite_pkt.type = MSG_INVITE_RECEIVED;
            invite_pkt.payload.invite.lobby_id = lobby->id;
            strncpy(invite_pkt.payload.invite.room_name, lobby->name, MAX_ROOM_NAME - 1);
            strncpy(invite_pkt.payload.invite.host_name, lobby->host_username, MAX_USERNAME - 1);
            strncpy(invite_pkt.payload.invite.access_code, lobby->access_code, 7); 
            invite_pkt.payload.invite.game_mode = lobby->game_mode;
            
            send_response(target_socket, &invite_pkt);
            
            response.type = MSG_NOTIFICATION;
            snprintf(response.message, sizeof(response.message), "Invitation sent to %s", pkt->target_display_name);
            send_response(socket_fd, &response);
            log_event("INVITE", "%s invited %s to Room %d", client->username, pkt->target_display_name, lobby->id);
        }
    } else {
        response.type = MSG_NOTIFICATION;
        snprintf(response.message, sizeof(response.message), "User %s not found or offline", pkt->target_display_name);
        send_response(socket_fd, &response);
    }
}
