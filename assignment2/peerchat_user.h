/**
 * peerchat_user.h
 * 
 * Author: Joseph Cumbo (jwc6999)
 */

#ifndef PEERCHAT_USER_INCLUDED
#define PEERCHAT_USER_INCLUDED

#include <stdint.h>

#include "peerchat_utility.h"

///////////////////////////////////////////////////////////
// User structs
///////////////////////////////////////////////////////////

typedef enum {
    USERSTATE_PENDING,
    USERSTATE_ACTIVE
} UserState;

typedef struct
{
    char username[USERNAME_LENGTH];
    uint8_t state;     // State the user is in
    int32_t socket;    // Socket the peer is connected to
    uint32_t address;  // IPv4 the peer connected from
    uint16_t port;     // Port peer is listening on
    uint32_t zip_code; // Zip of peer
    uint8_t age;       // Age of peer
} User;

///////////////////////////////////////////////////////////
// UserList structs
///////////////////////////////////////////////////////////

typedef struct {
    User users[MAX_PEERS];         // Array of users
    uint32_t length;               // Total number users
    FileDescriptorSet *master_fds; // Associated file descriptor set
} UserList;

///////////////////////////////////////////////////////////
// UserList functions
///////////////////////////////////////////////////////////

/**
 * Initializes a userlist.
 */
void userlist_initialize(UserList *list, FileDescriptorSet *master_fds);

/**
 * Prints users matching the given age.
 */
void userlist_print_by_age(UserList *list, uint8_t age);

/**
 * Prints users matching the given zip code.
 */
void userlist_print_by_zip(UserList *list, uint32_t zip_code);

/**
 * Prints users matching the given state.
 */
void userlist_print_by_state(UserList *list, uint8_t state);

/**
 * Returns true if the userlist has a peer with the given port and address.
 */
bool userlist_has_user(UserList *list, uint16_t port, uint32_t address);

/**
 * Finds the first user in the array of users by the given socket. Returns NULL
 * if no user was found with that socket.
 */
User *userlist_get_by_socket(UserList *list, int32_t socket);

/**
 * Adds the given user to the userlist. Returns a pointer to the added peer,
 * or NULL if the list is full.
 */
User *userlist_add(UserList *list, User *user);

/**
 * Disconnects and removes the user with the given socket from the userlist.
 */
void userlist_remove_by_socket(UserList *list, int32_t socket);

/**
 * Disconnects and removes all users from the userlist.
 */
void userlist_remove_all(UserList *list);

///////////////////////////////////////////////////////////
// User functions
///////////////////////////////////////////////////////////

/**
 * Prints the data for a user.
 */
void user_print(User *user);

/**
 * Sets the user's state to pending. This requires the minimal argument count
 * to setup the pending state.
 */
void user_set_pending(User *user, int32_t socket, uint32_t address);

/**
 * Sets the user's state to active. This requires the minimal argument count to
 * setup the active state.
 */
void user_set_active(User *user, char *username, uint16_t port, uint32_t zip_code, uint8_t age);

/**
 * Reads and parses command line arguments into a user.
 */
void user_parse_arguments(User *user, int argc, char *argv[]);

#endif