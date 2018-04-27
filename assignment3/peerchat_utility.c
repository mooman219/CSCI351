/**
 * peerchat_utility.c
 * 
 * Author: Joseph Cumbo (jwc6999)
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "peerchat_utility.h"

///////////////////////////////////////////////////////////
// FileDescriptorSet functions
///////////////////////////////////////////////////////////

void filedescriptorset_add(FileDescriptorSet *set, int32_t file_descriptor) {
    FD_SET(file_descriptor, &set->master_fds);
    set->length = file_descriptor >= set->length ? file_descriptor + 1 : set->length;
}

void filedescriptorset_remove(FileDescriptorSet *set, int32_t file_descriptor) {
    FD_CLR(file_descriptor, &set->master_fds);
}

void filedescriptorset_reset(FileDescriptorSet *set) {
    FD_ZERO(&set->master_fds);
    set->length = 0;
}

fd_set filedescriptorset_select(FileDescriptorSet *set) {
    fd_set read_fds = set->master_fds;
    // Block until a file descriptor that we set has data to consume.
    // We don't care about write_fds, except_fds, and timeout.
    if (select(set->length, &read_fds, NULL, NULL, NULL) == -1) {
        // Debug error message. Since we're not setting a timeout, select
        // should never return -1.
        printf("[Error: Select failed]\n");
        exit(EXIT_FAILURE);
    }
    return read_fds;
}

///////////////////////////////////////////////////////////
// Utility functions
///////////////////////////////////////////////////////////

bool starts_with(const char *source, const char *prefix) {
    if (strncmp(source, prefix, strlen(prefix)) == 0) {
        return true;
    }
    return false;
}

uint32_t remove_delimiters(char *source) {
    uint32_t i;
    for (i = 0; source[i] != 0; i++) {
        if (source[i] == '\r' || source[i] == '\n') {
            source[i] = '\0';
            return i;
        }
    }
    return i;
}

char *ip4_to_string(uint32_t ip4_address) {
    struct in_addr address;
    address.s_addr = ip4_address;
    return inet_ntoa(address);
}
