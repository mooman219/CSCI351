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

typedef struct
{
    char username[USERNAME_LENGTH];
    uint32_t address;  // IPv4 the peer connected from
    uint16_t port;     // Port peer is listening on
    uint32_t zip_code; // Zip of peer
    uint8_t age;       // Age of peer
} User;

///////////////////////////////////////////////////////////
// UserList structs
///////////////////////////////////////////////////////////

typedef struct {
    User users[MAX_PEERS]; // Array of users
    uint32_t length;       // Total number users
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
 * Prints all users.
 */
void userlist_print(UserList *list);

/**
 * Returns true if the userlist has a peer with the given port and address.
 */
bool userlist_has_user(UserList *list, uint16_t port, uint32_t address);

/**
 * Adds the given user to the userlist. Returns a pointer to the added peer,
 * or NULL if the list is full.
 */
User *userlist_add(UserList *list, char *username, uint16_t port, uint32_t address, uint32_t zip_code, uint8_t age);

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
 * Reads and parses command line arguments into a user.
 */
void user_parse_arguments(User *user, int argc, char *argv[]);

#endif