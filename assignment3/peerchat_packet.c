/**
 * peerchat_packet.c
 * 
 * Author: Joseph Cumbo (jwc6999)
 */

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "peerchat_packet.h"
#include "peerchat_user.h"
#include "peerchat_utility.h"

///////////////////////////////////////////////////////////
// Packet functions
///////////////////////////////////////////////////////////

PacketMessage message_instance;

PacketMessage *packet_message(User *user, char *message) {
    message_instance.type = PACKET_MESSAGE;
    strncpy(message_instance.username, user->username, USERNAME_LENGTH);
    strncpy(message_instance.message, message, MESSAGE_LENGTH);
    return &message_instance;
}

PacketJoin join_instance;

PacketJoin *packet_join(User *user, UserList *list, uint16_t port, uint32_t address) {
    // Join
    join_instance.type = PACKET_JOIN;
    strncpy(join_instance.username, user->username, USERNAME_LENGTH);
    join_instance.port = user->port;
    join_instance.age = user->age;
    join_instance.zip_code = user->zip_code;

    // Peer
    uint32_t x = 0;
    for (uint32_t i = 0; i < list->length; i++) {
        User *user = &list->users[i];
        if (user->port != port || user->address != address) {
            join_instance.peers[x].address = user->address;
            join_instance.peers[x].port = user->port;
            x += 1;
        }
    }
    join_instance.peer_length = x;
    return &join_instance;
}

PacketLeave leave_instance;

PacketLeave *packet_leave(User *user) {
    leave_instance.type = PACKET_LEAVE;
    strncpy(leave_instance.username, user->username, USERNAME_LENGTH);
    leave_instance.port = user->port;
    return &leave_instance;
}

void packet_send_direct(int32_t socket, void *packet, uint32_t size, uint16_t port, uint32_t address) {
    // Configure destination
    struct sockaddr_in destination;
    destination.sin_family = AF_INET;
    destination.sin_port = htons(port);
    destination.sin_addr.s_addr = address;

    // Send the payload
    if (sendto(socket, packet, size, 0, (struct sockaddr *)&destination, sizeof(destination)) < 0) {
        printf("[Error: Send failure to %s]\n", ip4_to_string(address));
    }
}

void packet_send(int32_t socket, void *packet, uint32_t size, User *user) {
    packet_send_direct(socket, packet, size, user->port, user->address);
}

void packet_send_all(int32_t socket, void *packet, uint32_t size, UserList *list) {
    for (uint32_t i = 0; i < list->length; i++) {
        packet_send(socket, packet, size, &list->users[i]);
    }
}