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
    User self;                    // User data for the primary user
    UserList peers;               // Active peers
    FileDescriptorSet master_fds; // The master file descriptor set
} Peerchat;

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
 * Handle a new connection from the accept_socket.
 */
void peerchat_accept(Peerchat *state, int32_t accept_socket) {
    struct sockaddr_in address;
    socklen_t address_length;
    // Accept the connection
    int32_t peer_socket = accept(accept_socket, (struct sockaddr *)&address, &address_length);
    // Fail if we were unable to create the socket
    if (peer_socket < 0) {
        printf("[Error: Accept failure - Unable to accept peer connection]\n");
        exit(EXIT_FAILURE);
    }

    // Mark the peer as pending
    User peer;
    user_set_pending(&peer, peer_socket, address.sin_addr.s_addr);
    // Add the peer to the known peers
    userlist_add(&state->peers, &peer);
    // Send our identity to peer
    packet_send(packet_identity(&state->self), &peer);
}

/**
 * Establish a connection the target address/port. Returns the added peer or
 * NULL if there was an error.
 */
User *peerchat_connect(Peerchat *state, uint16_t port, uint32_t address) {
    // Setup socket we're using to connect
    int32_t sock = socket(
        AF_INET,     // Use IPv4 addresses
        SOCK_STREAM, // This specifies TCP
        IPPROTO_TCP  // We're using the TCP protocol
    );
    // Fail if we were unable to create the socket
    if (sock < 0) {
        printf("[Join Failure - Unable to create socket]\n");
        return NULL;
    }
    // Destination setup
    struct sockaddr_in destination;
    destination.sin_family = AF_INET;
    destination.sin_port = htons(port);
    destination.sin_addr.s_addr = address;
    // Connect to destination
    if (connect(sock, (struct sockaddr *)&destination, sizeof(destination)) < 0) {
        printf("[Join Failure - Unable to establish connection]\n");
        return NULL;
    }
    // Peer handshake
    User peer;
    user_set_pending(&peer, sock, address);            // Mark the peer as pending
    packet_send(packet_identity(&state->self), &peer); // Send our identity to peer
    return userlist_add(&state->peers, &peer);         // Add the peer to the known peers
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
            userlist_print_by_state(&state->peers, USERSTATE_ACTIVE);
        }
        // Disconnect from all peers
        else if (starts_with(line, "/leave")) {
            userlist_remove_all(&state->peers);
            printf("[Left chat]\n");
        }
        // Send the chat message
        else {
            packet_send_all(packet_message(line), &state->peers);
        }
    }
}

/**
 * Handle reading data in from a peer.
 */
void peerchat_handle_peer_data(Peerchat *state, int32_t peer_socket) {
    // Find the associated peer
    User *peer = userlist_get_by_socket(&state->peers, peer_socket);
    // Read data from the peer socket
    Packet packet;
    ssize_t bytes_read = recv(peer_socket, &packet, sizeof(Packet), 0);
    // If no data was read, but the peer's file descriptor was set, then
    // the connection was closed.
    if (bytes_read <= 0) {
        userlist_remove_by_socket(&state->peers, peer_socket);
        return;
    }
    // Otherwise, we have data to parse from the client.
    switch (packet.type) {
        case PAYLOAD_MESSAGE:
            // Only display messages from active users
            if (peer->state == USERSTATE_ACTIVE) {
                printf(
                    "%s: %s\n",
                    peer->username,
                    packet.payload.message.message);
            }
            break;
        case PAYLOAD_IDENTITY:
            // Only allow pending users to identify
            if (peer->state == USERSTATE_PENDING) {
                packet_send(packet_peers(&state->peers, peer->socket), peer);
                user_set_active(
                    peer,
                    packet.payload.identity.username,
                    packet.payload.identity.port,
                    packet.payload.identity.zip_code,
                    packet.payload.identity.age);
            }
            break;
        case PAYLOAD_PEERS:
            for (uint32_t i = 0; i < packet.payload.peers.length; i++) {
                uint16_t port = packet.payload.peers.peers[i].port;
                uint32_t address = packet.payload.peers.peers[i].address;
                if (!userlist_has_user(&state->peers, port, address)) {
                    peerchat_connect(state, port, address);
                }
            }
            break;
    }
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

    // Setup socket that our peers will connect to
    int32_t accept_socket = socket(
        AF_INET,     // Use IPv4 addresses
        SOCK_STREAM, // This specifies TCP
        IPPROTO_TCP  // We're using the TCP protocol
    );
    // Fail if we were unable to create the socket
    if (accept_socket <= 0) {
        printf("[Error: Unable to create accept socket]\n");
        exit(EXIT_FAILURE);
    }
    // Configure the socket to allow multiple connections
    int32_t option_true = true;
    if (setsockopt(accept_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&option_true, sizeof(int32_t)) < 0) {
        printf("[Error: Unable to configure socket]\n");
        exit(EXIT_FAILURE);
    }
    // Configure socket address
    struct sockaddr_in accept_address;
    memset(&accept_address, 0, sizeof(accept_address)); // 0 out the address
    accept_address.sin_family = AF_INET;
    accept_address.sin_port = htons(state.self.port); // Port number.
    accept_address.sin_addr.s_addr = htonl(INADDR_ANY);
    // Bind the socket address to our accept socket
    if (bind(accept_socket, (struct sockaddr *)&accept_address, sizeof(accept_address)) < 0) {
        printf("[Error: Unable to bind socket]\n");
        exit(EXIT_FAILURE);
    }
    // Specify how many pending connections can be buffered
    if (listen(accept_socket, 10) < 0) {
        printf("[Error: Unable to set max pending connections]\n");
        exit(EXIT_FAILURE);
    }
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
            // If the file descriptor is accept_socket, handle a new connection
            else if (i == accept_socket) {
                peerchat_accept(&state, accept_socket);
            }
            // Else we're receiving data from an already established connection
            else {
                peerchat_handle_peer_data(&state, i);
            }
        }
    }

    return 0;
}
