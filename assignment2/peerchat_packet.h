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

typedef struct
{
    char message[MESSAGE_LENGTH];
} PayloadMessage;

typedef struct
{
    char username[USERNAME_LENGTH];
    uint16_t port;
    uint32_t zip_code;
    uint8_t age;
    struct {
        uint32_t address;
        uint16_t port;
    } peers[MAX_PEERS];
    uint32_t peer_length;
} PayloadIdentity;

typedef enum {
    PAYLOAD_MESSAGE,
    PAYLOAD_IDENTITY
} PayloadType;

typedef union {
    PayloadMessage message;
    PayloadIdentity identity;
} PacketPayload;

typedef struct
{
    uint8_t type;
    PacketPayload payload;
} Packet;

///////////////////////////////////////////////////////////
// Packet functions
///////////////////////////////////////////////////////////

/**
 * Prepares a temporary packet with the message payload.
 * 
 * Successive calls to this function overwrite the returned packet.
 */
Packet *packet_message(char *message);

/**
 * Prepares a temporary packet with the identity payload.
 * 
 * Successive calls to this function overwrite the returned packet.
 */
Packet *packet_identity(User *user, UserList *list, int32_t ignore_socket);

/**
 * Sends a packet to the user.
 */
void packet_send(Packet *packet, User *user);

/**
 * Sends a packet to all users in the userlist.
 */
void packet_send_all(Packet *packet, UserList *list);

#endif