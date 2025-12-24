#ifndef CLIENT_PACKET_HANDLERS_H
#define CLIENT_PACKET_HANDLERS_H

#include "../common/protocol.h"

// Packet handler function type
typedef void (*PacketHandler)(ServerPacket *pkt);

// Initialize packet handler system
void init_client_packet_handlers(void);

// Dispatch packet to appropriate handler
void handle_server_packet(ServerPacket *pkt);

// Individual packet handler declarations
void handle_auth_response(ServerPacket *pkt);
void handle_lobby_list(ServerPacket *pkt);
void handle_lobby_update(ServerPacket *pkt);
void handle_game_state(ServerPacket *pkt);
void handle_error(ServerPacket *pkt);
void handle_friend_list_response(ServerPacket *pkt);
void handle_friend_response(ServerPacket *pkt);
void handle_profile_response(ServerPacket *pkt);
void handle_leaderboard_response(ServerPacket *pkt);
void handle_notification(ServerPacket *pkt);
void handle_chat(ServerPacket *pkt);
void handle_invite_received(ServerPacket *pkt);

#endif // CLIENT_PACKET_HANDLERS_H
