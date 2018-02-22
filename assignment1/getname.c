#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

// QTYPE Values
#define QTYPE_A 1      // a host address
#define QTYPE_NS 2     // an authoritative name server
#define QTYPE_MD 3     // a mail destination (Obsolete - use MX)
#define QTYPE_MF 4     // a mail forwarder (Obsolete - use MX)
#define QTYPE_CNAME 5  // the canonical name for an alias
#define QTYPE_SOA 6    // marks the start of a zone of authority
#define QTYPE_MB 7     // a mailbox domain name (EXPERIMENTAL)
#define QTYPE_MG 8     // a mail group member (EXPERIMENTAL)
#define QTYPE_MR 9     // a mail rename domain name (EXPERIMENTAL)
#define QTYPE_NULL 10  // a null RR (EXPERIMENTAL)
#define QTYPE_WKS 11   // a well known service description
#define QTYPE_PTR 12   // a domain name pointer
#define QTYPE_HINFO 13 // host information
#define QTYPE_MINFO 14 // mailbox or mail list information
#define QTYPE_MX 15    // mail exchange
#define QTYPE_TXT 16   // text strings

// QCLASS Values
#define QCLASS_IN 1 // the Internet
#define QCLASS_CS 2 // the CSNET class (Obsolete - used only for examples in some obsolete RFCs)
#define QCLASS_CH 3 // the CHAOS class
#define QCLASS_HS 4 // Hesiod [Dyer 87]

#define DNS_PORT 53 // port to query DNS on

//DNS header structure
typedef struct
{
    // Bits 00-15 | 2 bytes
    uint16_t id; // identification number

    // Bits 16-23 | 1 byte (little-endian)
    uint8_t rd : 1;     // recursion desired
    uint8_t tc : 1;     // truncated message
    uint8_t aa : 1;     // authoritive answer
    uint8_t opcode : 4; // purpose of message
    uint8_t qr : 1;     // query/response flag

    // Bits 24-31 | 1 byte (little-endian)
    uint8_t rcode : 4; // response code
    uint8_t z : 3;     // its z! reserved
    uint8_t ra : 1;    // recursion available

    // Bits 32-47 | 2 bytes
    uint16_t q_count; // number of question entries
    // Bits 48-63 | 2 bytes
    uint16_t ans_count; // number of answer entries
    // Bits 64-79 | 2 bytes
    uint16_t auth_count; // number of authority entries
    // Bits 80-95 | 2 bytes
    uint16_t add_count; // number of resource entries
} DNS_Header;

// constant sized fields of query structure
typedef struct
{
    uint16_t qtype;
    uint16_t qclass;
} Question;

// constant sized fields of the resource record structure
#pragma pack(push, 1)
typedef struct
{
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t data_len;
} R_Data;
#pragma pack(pop)

// pointers to resource record contents
typedef struct
{
    char *name;
    R_Data *resource;
    char *rdata;
} RES_Record;

// Structure of a Query
typedef struct
{
    char *name;
    Question *ques;
} Query;

void do_query(char *domain, char *addr);
void strcpy_to_dns(char *dest, char *src);
int strcpy_from_dns(char *dest, uint8_t *buffer, uint8_t *pointer);

int main(int argc, char *argv[])
{
    // Argument parsing
    if (argc != 3)
    {
        printf("Usage: getname (name) (ipaddr)\n");
        return 0;
    }
    char *domain = argv[1];
    char *addr = argv[2];

    // do the query
    do_query(domain, addr);

    return 0;
}

void do_query(char *domain, char *address)
{
    uint8_t payload[8192];
    int payload_counter = 0;

    // DNS request setup
    DNS_Header *header = (DNS_Header *)(payload);
    header->id = htons(getpid());
    header->qr = 0;
    header->opcode = 0;
    header->aa = 0;
    header->tc = 0;
    header->rd = 1;
    header->ra = 0;
    header->z = 0;
    header->rcode = 0;
    header->q_count = htons(1);
    header->ans_count = 0;
    header->auth_count = 0;
    header->add_count = 0;
    payload_counter += sizeof(DNS_Header); // increment by DNS_Header size
    printf("Payload counter A: %d\n", payload_counter);

    // Question name setup
    char *qname = (char *)(payload + payload_counter);
    strcpy_to_dns(qname, domain);
    payload_counter += strlen(qname) + 1; // increment by strlen plus null terminator
    printf("Payload counter B: %d\n", payload_counter);

    // Question setup
    Question *question = (Question *)(payload + payload_counter);
    question->qtype = htons(QTYPE_A);
    question->qclass = htons(QCLASS_IN);
    payload_counter += sizeof(Question); // increment by Question size
    printf("Payload counter C: %d\n", payload_counter);

    // Destination setup
    struct sockaddr_in destination;
    destination.sin_family = AF_INET;
    destination.sin_port = htons(DNS_PORT);
    destination.sin_addr.s_addr = inet_addr(address);
    int i = sizeof(destination);

    // Network send
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    printf("Info: Sending packet with size %d...\n", payload_counter);
    if (sendto(sock, payload, payload_counter, 0, (struct sockaddr *)&destination, i) < 0)
    {
        perror("Error: Failed to send packet.\n");
        return;
    }
    printf("Info: Sent packet.\n");

    // Network receive
    printf("Info: Receiving packet...\n");
    if (recvfrom(sock, (char *)payload, 8192, 0, (struct sockaddr *)&destination, (socklen_t *)&i) < 0)
    {
        perror("Error: Failed to receive packet.\n");
        return;
    }
    printf("Info: Received packet.\n");

    if (ntohs(header->ans_count) < 1)
    {
        perror("Error: No answer.\n");
        return;
    }

    printf("Info: The response contains\n");
    printf("  %d Questions.\n", ntohs(header->q_count));
    printf("  %d Answers.\n", ntohs(header->ans_count));

    // Answer data
    RES_Record answer;
    char answer_name[256];
    char answer_data[256];

    // Answer name parsing
    answer.name = answer_name;
    payload_counter += strcpy_from_dns(answer.name, payload, (payload + payload_counter));
    printf("Info: Name: [%s]\n", answer.name);

    // Answer resource parsing
    answer.resource = (R_Data *)(payload + payload_counter);
    answer.resource->type = ntohs(answer.resource->type);
    answer.resource->class = ntohs(answer.resource->class);
    answer.resource->ttl = ntohs(answer.resource->ttl);
    answer.resource->data_len = ntohs(answer.resource->data_len);
    payload_counter += sizeof(R_Data);
    printf("Info: R_Data contains\n");
    printf("  Type: %d\n", answer.resource->type);
    printf("  Class: %d\n", answer.resource->class);
    printf("  TTL: %d\n", answer.resource->ttl);
    printf("  Data Length: %d\n", answer.resource->data_len);

    answer.rdata = answer_data;
    if (answer.resource->type == 1) // ipv4 address
    {
        int length = answer.resource->data_len;
        uint8_t *pointer = payload + payload_counter;
        sprintf(answer.rdata, "%u.%u.%u.%u", *(pointer), *(pointer + 1), *(pointer + 2), *(pointer + 3));
        payload_counter += length;
    }
    else
    {
        payload_counter += strcpy_from_dns(answer.rdata, payload, (payload + payload_counter));
    }
    printf("Info: Address: [%s]\n", answer.rdata);

    return;
}

int strcpy_from_dns(char *dest, uint8_t *buffer, uint8_t *pointer)
{
    unsigned int p = 0;
    unsigned int jumped = 0;
    unsigned int offset;
    unsigned int count = 1;
    dest[0] = '\0';

    //read the names in 3www6google3com format
    while (*pointer != 0)
    {
        if (*pointer >= 0xC0) // 1100 000
        {
            offset = (*pointer) * 256 + *(pointer + 1) - 0xC000; //1100 0000 0000 0000
            pointer = buffer + offset - 1;
            jumped = 1; //we have jumped to another location so counting wont go up!
        }
        else
        {
            dest[p++] = *pointer;
        }
        pointer++;
        if (jumped == 0)
        {
            count++; //if we havent jumped to another location then we can count up
        }
    }
    dest[p] = '\0'; //string complete

    if (jumped == 1)
    {
        count++; //number of steps we actually moved forward in the packet
    }

    // Convert from DNS
    int i = 0;
    for (; i < strlen(dest); i++)
    {
        p = dest[i];
        for (int j = 0; j < (int)p; j++)
        {
            dest[i] = dest[i + 1];
            i++;
        }
        dest[i] = '.';
    }
    dest[i - 1] = '\0'; //remove the last dot
    return count;
}

void strcpy_to_dns(char *dest, char *src)
{
    int length = strlen(src) + 1;
    int last = 0;
    strcpy(dest + 1, src);
    for (int i = 1; i <= length; i++)
    {
        if (dest[i] == '.' || dest[i] == '\0')
        {
            dest[last] = (i - last - 1);
            last = i;
        }
    }
}
