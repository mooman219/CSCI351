/**
 * peerchat_user.c
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

#include "peerchat_user.h"
#include "peerchat_utility.h"

///////////////////////////////////////////////////////////
// UserList functions
///////////////////////////////////////////////////////////

void userlist_initialize(UserList *list, FileDescriptorSet *master_fds) {
    list->length = 0;
}

void userlist_print_by_age(UserList *list, uint8_t age) {
    for (uint32_t i = 0; i < list->length; i++) {
        User *user = &list->users[i];
        if (user->age == age) {
            user_print(user);
        }
    }
}

void userlist_print_by_zip(UserList *list, uint32_t zip_code) {
    for (uint32_t i = 0; i < list->length; i++) {
        User *user = &list->users[i];
        if (user->zip_code == zip_code) {
            user_print(&list->users[i]);
        }
    }
}

void userlist_print(UserList *list) {
    for (uint32_t i = 0; i < list->length; i++) {
        user_print(&list->users[i]);
    }
}

bool userlist_has_user(UserList *list, uint16_t port, uint32_t address) {
    for (uint32_t i = 0; i < list->length; i++) {
        User *user = &list->users[i];
        if (user->port == port && user->address == address) {
            return true;
        }
    }
    return false;
}

User *userlist_add(UserList *list, char *username, uint16_t port, uint32_t address, uint32_t zip_code, uint8_t age) {
    if (list->length == MAX_PEERS - 1) {
        printf("[Warning: Attempted to add user while at capacity]\n");
        return NULL;
    }
    // If the address is 0.0.0.0, set it to 127.0.0.1 (network order)
    if (address == 0) {
        address = 0x100007F;
    }
    // Get the peer
    User *slot = &list->users[list->length];
    strncpy(slot->username, username, USERNAME_LENGTH);
    slot->port = port;
    slot->address = address;
    slot->zip_code = zip_code;
    slot->age = age;
    list->length += 1;
    return slot;
}

void userlist_remove_by_connection(UserList *list, uint16_t port, uint32_t address) {
    for (uint32_t i = 0; i < list->length; i++) {
        User *user = &list->users[i];
        if (user->port == port && user->address == address) {
            printf("[%s@%s:%hu left the chat]\n", user->username, ip4_to_string(address), port);
            list->length -= 1;
            *user = list->users[list->length];
        }
    }
}

void userlist_remove_all(UserList *list) {
    for (uint32_t i = 0; i < list->length; i++) {
        User user = list->users[i];
        printf("[%s@%s left the chat]\n", user.username, ip4_to_string(user.address));
    }
    list->length = 0;
}

///////////////////////////////////////////////////////////
// User functions
///////////////////////////////////////////////////////////

void user_print(User *state) {
    printf(
        "[Username: %s | Zip: %u | Age: %hhu]\n",
        state->username,
        state->zip_code,
        state->age);
}

void user_parse_arguments(User *state, int argc, char *argv[]) {
    if (argc == 6) {
        // Parse username
        strncpy(state->username, argv[3], USERNAME_LENGTH);
        state->username[USERNAME_LENGTH - 1] = '\0';
        state->port = atoi(argv[2]);     // Parse port
        state->zip_code = atoi(argv[4]); // Parse zip
        state->age = atoi(argv[5]);      // Parse age
    } else if (argc == 4) {
        // Parse username
        strncpy(state->username, argv[1], USERNAME_LENGTH);
        state->username[USERNAME_LENGTH - 1] = '\0';
        state->port = DEFAULT_PORT;      // Default port
        state->zip_code = atoi(argv[2]); // Parse zip
        state->age = atoi(argv[3]);      // Parse age
    } else {
        printf("[Invalid command - Expected: peerchat [-p <port>] <username> <zip code> <age>]\n");
        exit(EXIT_FAILURE);
    }
}