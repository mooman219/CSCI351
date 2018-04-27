/**
 * peerchat.c
 * 
 * Author: Joseph Cumbo (jwc6999)
 */

#include <arpa/inet.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "peerchat_packet.h"
#include "peerchat_user.h"
#include "peerchat_utility.h"

///////////////////////////////////////////////////////////
// Peerchat structs
///////////////////////////////////////////////////////////

typedef struct
{
    int32_t socket;               // UDP socket
    User self;                    // User data for the primary user
    UserList peers;               // Active peers
    FileDescriptorSet master_fds; // The master file descriptor set
} Peerchat;

///////////////////////////////////////////////////////////
// Peerchat headers
///////////////////////////////////////////////////////////

/**
 * Handle reading message data in from a peer.
 */
void peerchat_read_message(Peerchat *state, PacketMessage *packet, uint32_t address);

/**
 * Handle reading join data in from a peer.
 */
void peerchat_read_join(Peerchat *state, PacketJoin *packet, uint32_t address);

/**
 * Handle reading leave data in from a peer.
 */
void peerchat_read_leave(Peerchat *state, PacketLeave *packet, uint32_t address);

///////////////////////////////////////////////////////////
// Peerchat functions
///////////////////////////////////////////////////////////

/**
 * Initializes a peerchat.
 */
void peerchat_initialize(Peerchat *state) {
    memset(state, 0, sizeof(Peerchat));
    filedescriptorset_reset(&state->master_fds);
    userlist_initialize(&state->peers, &state->master_fds);
}

/**
 * Establish a connection the target address/port.
 */
void peerchat_connect(Peerchat *state, uint16_t port, uint32_t address) {
    // Check if we already are connected to the peer
    if (userlist_has_user(&state->peers, port, address)) {
        return;
    }
    // Send our join
    PacketJoin *join = packet_join(&state->self, &state->peers, port, address);
    packet_send_direct(state->socket, join, sizeof(PacketJoin), port, address);
}

/**
 * Handle input from stdin. This should exhaust stdin such that select does
 * not re-trigger for stdin.
 */
void peerchat_handle_input(Peerchat *state) {
    // Read the line from stdin
    char line[512];
    fgets(line, 512, stdin);
    // Remove delimiters
    uint32_t line_length = remove_delimiters(line);
    // Ignore empty lines
    if (line_length > 0) {
        // Disconnect from all peers and exit the program
        if (starts_with(line, "/exit")) {
            // Send leave packet
            PacketLeave *packet = packet_leave(&state->self);
            packet_send_all(state->socket, packet, sizeof(PacketLeave), &state->peers);
            // Cleanup userlist
            userlist_remove_all(&state->peers);
            printf("[Exited]\n");
            exit(EXIT_SUCCESS);
        }
        // Connect to the target peer
        // Format: /join [-p <port>] <address>
        else if (starts_with(line, "/join")) {
            if (state->peers.length > 0) {
                // Fail if we're already connected.
                printf("[Join Failure - Already connected to peers]\n");
                return;
            }
            // Parse the input line
            uint16_t port;
            uint32_t address;
            char address_buf[16];
            if (sscanf(line, "/join -p %hu %15s", &port, address_buf) == 2) {
            } else if (sscanf(line, "/join %15s", address_buf) == 1) {
                port = DEFAULT_PORT;
            } else {
                printf("[Join Failure - Expected: /join [-p <port>] <address>]\n");
                return;
            }
            address = inet_addr(address_buf);
            // Connect
            peerchat_connect(state, port, address);
        }
        // Print all users with the matching age
        // Format: /age <number>
        else if (starts_with(line, "/age")) {
            uint8_t age;
            if (sscanf(line, "/age %hhu", &age) == 1) {
                if (state->self.age == age) {
                    user_print(&state->self);
                }
                userlist_print_by_age(&state->peers, age);
            } else {
                printf("[Expected: /age <number>]\n");
            }
        }
        // Print all users with the matching zip code
        // Format: /zip <number>
        else if (starts_with(line, "/zip")) {
            uint32_t zip_code;
            if (sscanf(line, "/zip %u", &zip_code) == 1) {
                if (state->self.zip_code == zip_code) {
                    user_print(&state->self);
                }
                userlist_print_by_zip(&state->peers, zip_code);
            } else {
                printf("[Expected: /zip <number>]\n");
            }
        }
        // Print all active users
        else if (starts_with(line, "/who")) {
            user_print(&state->self);
            userlist_print(&state->peers);
        }
        // Disconnect from all peers
        else if (starts_with(line, "/leave")) {
            // Send leave packet
            PacketLeave *packet = packet_leave(&state->self);
            packet_send_all(state->socket, packet, sizeof(PacketLeave), &state->peers);
            // Cleanup userlist
            userlist_remove_all(&state->peers);
            printf("[Left chat]\n");
        }
        // Send the chat message
        else {
            PacketMessage *packet = packet_message(&state->self, line);
            packet_send_all(state->socket, packet, sizeof(PacketMessage), &state->peers);
        }
    }
}

void peerchat_read(Peerchat *state) {
    // Read data from the socket
    struct sockaddr_in addr;
    uint32_t addr_size = 0;
    uint8_t buffer[2048];
    ssize_t bytes_read = recvfrom(state->socket, (char *)buffer, 2048, 0, (struct sockaddr *)&addr, &addr_size);
    if (bytes_read <= 0) {
        printf("[Read Failure - No bytes received]\n");
        return;
    }

    // If the address is 0.0.0.0, set it to 127.0.0.1 (network order)
    uint32_t address = addr.sin_addr.s_addr;
    if (address == 0) {
        address = 0x100007F;
    }

    // Switch on packet type
    switch (buffer[0]) {
        case PACKET_MESSAGE: {
            peerchat_read_message(state, (PacketMessage *)buffer, address);
            break;
        }
        case PACKET_JOIN: {
            peerchat_read_join(state, (PacketJoin *)buffer, address);
            break;
        }
        case PACKET_LEAVE: {
            peerchat_read_leave(state, (PacketLeave *)buffer, address);
            break;
        }
    }
}

void peerchat_read_message(Peerchat *state, PacketMessage *packet, uint32_t address) {
    // Display the message
    packet->username[USERNAME_LENGTH - 1] = '\0';
    packet->message[MESSAGE_LENGTH - 1] = '\0';
    printf("<%s> %s\n", packet->username, packet->message);
}

void peerchat_read_join(Peerchat *state, PacketJoin *packet, uint32_t address) {
    // Join message for the first connection
    if (state->peers.length == 0) {
        printf("[Joined chat with %u members]\n", packet->peer_length + 1);
    }
    // Check if we're already connected to that peer
    if (userlist_has_user(&state->peers, packet->port, address)) {
        return;
    }
    // Add the peer
    peerchat_connect(state, packet->port, address);
    User *peer = userlist_add(&state->peers, packet->username, packet->port, address, packet->zip_code, packet->age);
    printf("[%s@%s:%hu has joined (Zip: %u, Age: %hhu)]\n", peer->username, ip4_to_string(peer->address), packet->port, peer->zip_code, peer->age);
    // Connect to their peers
    packet->peer_length = packet->peer_length > MAX_PEERS ? MAX_PEERS : packet->peer_length;
    for (uint32_t i = 0; i < packet->peer_length; i++) {
        peerchat_connect(state, packet->peers[i].port, packet->peers[i].address);
    }
}

void peerchat_read_leave(Peerchat *state, PacketLeave *packet, uint32_t address) {
    // Remove the peer
    userlist_remove_by_connection(&state->peers, packet->port, address);
}

///////////////////////////////////////////////////////////
// Main function
///////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
    // Turn off standard out buffering
    setbuf(stdout, NULL);

    // Setup peerchat state
    Peerchat state;
    peerchat_initialize(&state);
    // Parse the command line arguments
    user_parse_arguments(&state.self, argc, argv);
    // Add stdin to the file descriptor set
    filedescriptorset_add(&state.master_fds, STDIN_FILENO);

    // Setup socket that our peers will send data to
    int32_t accept_socket = socket(
        AF_INET,    // Use IPv4 addresses
        SOCK_DGRAM, // This specifies UDP
        IPPROTO_UDP // We're using the UDP protocol
    );
    // Fail if we were unable to create the socket
    if (accept_socket <= 0) {
        printf("[Error: Unable to create data socket]\n");
        exit(EXIT_FAILURE);
    }

    // Configure socket address
    struct sockaddr_in accept_address;
    memset(&accept_address, 0, sizeof(accept_address)); // 0 out the address
    accept_address.sin_family = AF_INET;                // IPv4
    accept_address.sin_port = htons(state.self.port);   // Port number
    accept_address.sin_addr.s_addr = htonl(INADDR_ANY); // Accept any bind address
    // Bind the socket address to our accept socket
    if (bind(accept_socket, (struct sockaddr *)&accept_address, sizeof(accept_address)) < 0) {
        printf("[Error: Unable to bind socket, port already in use.]\n");
        exit(EXIT_FAILURE);
    }

    // Store the accept socket
    state.socket = accept_socket;
    // Add accept_socket to the master file descriptor list
    filedescriptorset_add(&state.master_fds, accept_socket);

    // Loop forever
    while (true) {
        fd_set read_fds = filedescriptorset_select(&state.master_fds);

        // Loop through our file descriptors
        for (int32_t i = 0; i < state.master_fds.length; i++) {
            // If the file descriptor is not set, skip to the next one
            if (!FD_ISSET(i, &read_fds)) continue;

            // If the file descriptor is stdin, handle input
            if (i == STDIN_FILENO) {
                peerchat_handle_input(&state);
            }
            // If the file descriptor is accept_socket, handle receiving data
            else if (i == accept_socket) {
                peerchat_read(&state);
            }
            // Else we're receiving data from an unknown file descriptor
            else {
                printf("[Error: Unknown file descriptor selected.]\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    return 0;
}
