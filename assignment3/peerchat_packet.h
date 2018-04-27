/**
 * peerchat_packet.h
 * 
 * Author: Joseph Cumbo (jwc6999)
 */

#ifndef PEERCHAT_PACKET_INCLUDED
#define PEERCHAT_PACKET_INCLUDED

#include <stdint.h>

#include "peerchat_user.h"
#include "peerchat_utility.h"

///////////////////////////////////////////////////////////
// Packet structs
///////////////////////////////////////////////////////////

typedef enum {
    PACKET_MESSAGE,
    PACKET_JOIN,
    PACKET_LEAVE,
} PacketType;

typedef struct
{
    uint8_t type;
    char username[USERNAME_LENGTH];
    char message[MESSAGE_LENGTH];
} PacketMessage;

typedef struct
{
    uint8_t type;
    char username[USERNAME_LENGTH];
    uint16_t port;
    uint32_t zip_code;
    uint8_t age;
    uint32_t peer_length;
    struct {
        uint32_t address;
        uint16_t port;
    } peers[MAX_PEERS];
} PacketJoin;

typedef struct
{
    uint8_t type;
    char username[USERNAME_LENGTH];
    uint16_t port;
} PacketLeave;

///////////////////////////////////////////////////////////
// Packet functions
///////////////////////////////////////////////////////////

/**
 * Prepares a temporary packet with the message payload.
 * 
 * Successive calls to this function overwrite the returned packet.
 */
PacketMessage *packet_message(User *user, char *message);

/**
 * Prepares a temporary packet with the join payload.
 * 
 * Successive calls to this function overwrite the returned packet.
 */
PacketJoin *packet_join(User *user, UserList *list, uint16_t port, uint32_t address);

/**
 * Prepares a temporary packet with the leave payload.
 * 
 * Successive calls to this function overwrite the returned packet.
 */
PacketLeave *packet_leave(User *user);

/**
 * Sends a packet directly to a port/address.
 */
void packet_send_direct(int32_t socket, void *packet, uint32_t size, uint16_t port, uint32_t address);

/**
 * Sends a packet to the user.
 */
void packet_send(int32_t socket, void *packet, uint32_t size, User *user);

/**
 * Sends a packet to all users in the userlist.
 */
void packet_send_all(int32_t socket, void *packet, uint32_t size, UserList *list);

#endif