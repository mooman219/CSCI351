/**
 * peerchat_utility.h
 * 
 * Author: Joseph Cumbo (jwc6999)
 */

#ifndef PEERCHAT_UTILITY_INCLUDED
#define PEERCHAT_UTILITY_INCLUDED

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////
// Global macros
///////////////////////////////////////////////////////////

#define MAX_PEERS 32
#define USERNAME_LENGTH 32
#define MESSAGE_LENGTH 128
#define DEFAULT_PORT 8129

///////////////////////////////////////////////////////////
// FileDescriptorSet structs
///////////////////////////////////////////////////////////

typedef struct {
    fd_set master_fds; // The file descriptor list
    uint32_t length;   // Total number of active read file descriptors
} FileDescriptorSet;

///////////////////////////////////////////////////////////
// FileDescriptorSet functions
///////////////////////////////////////////////////////////

/**
 * Adds the given file descriptor to the set.
 */
void filedescriptorset_add(FileDescriptorSet *set, int32_t file_descriptor);

/**
 * Removes the given file descriptor to the set.
 */
void filedescriptorset_remove(FileDescriptorSet *set, int32_t file_descriptor);

/**
 * Clears all tracked file descriptors.
 */
void filedescriptorset_reset(FileDescriptorSet *set);

/**
 * Selects on the tracked file descriptors. Returns the read file descriptor
 * set. 
 */
fd_set filedescriptorset_select(FileDescriptorSet *set);

///////////////////////////////////////////////////////////
// Utility functions
///////////////////////////////////////////////////////////

/**
 * Checks is the first sring starts with the second string.
 * 
 * @param source - The source string.
 * @param prefix - The prefix to check for.
 * @return - True if the source string starts with the prefix, false otherwise.
 */
bool starts_with(const char *source, const char *prefix);

/**
 * Removes \r and \n delimiters from the string in place. Returns the new
 * length of the source.
 */
uint32_t remove_delimiters(char *source);

/**
 * Parses a network ordered ipv4 value into a string. Returns the parsed ipv4
 * address as a string.
 * 
 * Successive calls to this function overwrite the returned string.
 */
char *ip4_to_string(uint32_t ip4_address);

#endif
