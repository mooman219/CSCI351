/**
 * peerchat_packet.c
 * 
 * Author: Joseph Cumbo (jwc6999)
 */

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "peerchat_packet.h"
#include "peerchat_user.h"
#include "peerchat_utility.h"

///////////////////////////////////////////////////////////
// Packet functions
///////////////////////////////////////////////////////////

Packet packet_global_temp;

Packet *packet_message(char *message) {
    packet_global_temp.type = PAYLOAD_MESSAGE;
    strncpy(packet_global_temp.payload.message.message, message, MESSAGE_LENGTH);
    return &packet_global_temp;
}

Packet *packet_identity(User *user, UserList *list, int32_t ignore_socket) {
    packet_global_temp.type = PAYLOAD_IDENTITY;
    // Identity
    strncpy(packet_global_temp.payload.identity.username, user->username, USERNAME_LENGTH);
    packet_global_temp.payload.identity.port = user->port;
    packet_global_temp.payload.identity.age = user->age;
    packet_global_temp.payload.identity.zip_code = user->zip_code;
    // Peer
    uint32_t x = 0;
    for (uint32_t i = 0; i < list->length; i++) {
        User *user = &list->users[i];
        if (user->state == USERSTATE_ACTIVE && user->socket != ignore_socket) {
            packet_global_temp.payload.identity.peers[x].address = user->address;
            packet_global_temp.payload.identity.peers[x].port = user->port;
            x += 1;
        }
    }
    packet_global_temp.payload.identity.peer_length = x;
    return &packet_global_temp;
}

void packet_send(Packet *packet, User *user) {
    if (send(user->socket, packet, sizeof(Packet), 0) < 0) {
        printf("[Error: Send failure to %s]\n", ip4_to_string(user->address));
    }
}

void packet_send_all(Packet *packet, UserList *list) {
    for (uint32_t i = 0; i < list->length; i++) {
        packet_send(packet, &list->users[i]);
    }
}