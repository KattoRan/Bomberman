/* client/network/network.h */
#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "../common/protocol.h"

// External socket
extern int sock;

// Functions
int connect_to_server(const char *server_ip, int port);
void disconnect_from_server();
int receive_server_packet(ServerPacket *out_packet);
void send_packet(int type, int data);
void process_server_packet(ServerPacket *pkt);

#endif