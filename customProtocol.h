/**
 * @file customProtocol.h
 * @author Benjamin Wang (bwang4@scu.edu, ID: 1179478)
 * @brief Client using customized protocol on top of UDP protocol for requesting
 *      identification from server for access permission to the cellular network.
 *      Header file defining the CUSTOM PROTOCOL
 * @version 0.2
 * @date 2022-03-05
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef CUSTOMPROTOCOL_H /* include guard */
#define CUSTOMPROTOCOL_H

// Debug flag
// #define DEBUGGING 0
#define PRINT_DATABASE 0

// library includes
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>

// Port and Hostname:
#define PORT 8080
#define HOSTNAME "localhost"

// Custom Protocol Meta-Settings
#define PACKET_GROUP_SIZE 5
#define ACK_TIMER_WAIT_TIME_MS 3000
#define ACK_TIMER_RETRY_COUNT 3
#define MAXLINE 1024
#define LINE_LENGTH 256
#define DEFAULT_VALUE 0
#define PACKET_OK 0

// Error Handling Testing:
#define ERROR_SPECIAL_CLIENT 123
#define ERR_OOS_MSG "Case 1: Out of Sequence segment number"
#define ERR_LEN_MSG "Case 2: Length Mismatch"
#define ERR_END_MSG "Case 3: End of Packet Missing"
#define ERR_DUP_MSG "Case 4: Duplicate Packet check done by comparing sequence number as specified in instruction"

// Custom Protocol Primitives
#define START_PACKET 0xFFFF
#define END_PACKET 0xFFFF
#define MAX_CLIENT_ID 0XFF
#define MAX_PACKET_SIZE 0xFF

// Custom Protocol Packet Types
typedef enum
{
    PACKET_DATA = 0xFFF1,
    PACKET_ACK,
    PACKET_REJECT
} PACKET_TYPE;
// Custom Protocol Reject Sub-Codes
typedef enum
{
    REJECT_OUT_OF_SEQUENCE = 0xFFF4,
    REJECT_LENGTH_MISMATCH,
    REJECT_END_OF_PACKET_MISSING,
    REJECT_DUPLICATE_PACKET
} REJECT_SUB_CODE;

// Custom Protocol Subscriber Technologies:
typedef enum {
    SUB_2G = 2,
    SUB_3G,
    SUB_4G,
    SUB_5G
} SUBSCRIBER_TECHNOLOGY;

#define SUBSCRIBER_PAYLOAD_SIZE 6
#define PHONE_NUMBER_SIZE 10

// Custom Protocol Subscriber Access Permission Request & Response Types:
typedef enum
{
    SUB_ACC_PER = 0xFFF8,
    SUB_NOT_PAID,
    SUB_NOT_EXIST,
    SUB_ACC_OK
} SUBSCRIBER_PACKET_TYPE;

#define SUB_ACC_PER_MSG "Subscriber Access Permission Request"
#define SUB_NOT_PAID_MSG "Subscriber Not Paid"
#define SUB_NOT_EXIST_MSG "Subscriber Not Exist"
#define SUB_ACC_OK_MSG "Subscriber Access Granted"

#define VERIFICATION_DATABASE_SIZE 100

// Custom Protocol Verification Database struct:
typedef struct {
    uint32_t src_sub_no;
    SUBSCRIBER_TECHNOLOGY technology;
    bool paid;
} __attribute__((packed)) verification_database_t;

// Custom Protocol Subscriber Access Permission Request & Response Packet struct:
typedef struct
{
    uint16_t start_packet;
    uint8_t client_id;
    SUBSCRIBER_PACKET_TYPE packet_type;
    uint8_t segment_no;
    uint8_t length;
    uint8_t technology;
    uint32_t src_sub_no;
    uint16_t end_packet;
} __attribute__((packed)) subscriber_packet_t; // Size 15

// Custom Protocol Packet Structure Sizes
#define PACKET_DATA_PAYLOAD_SIZE 255
// Custom Protocol Packet Structures:
typedef struct
{
    uint16_t start_packet;
    uint8_t client_id;
    PACKET_TYPE packet_type;
    uint8_t segment_no;
    uint8_t length;
    char payload[PACKET_DATA_PAYLOAD_SIZE];
    uint16_t end_packet;
} __attribute__((packed)) data_packet_t; // Size 266

typedef struct
{
    uint16_t start_packet;
    uint8_t client_id;
    PACKET_TYPE packet_type;
    uint8_t received_segment_no;
    uint16_t end_packet;
} __attribute__((packed)) ack_packet_t; // Size 8

typedef struct
{
    uint16_t start_packet;
    uint8_t client_id;
    PACKET_TYPE packet_type;
    REJECT_SUB_CODE sub_code;
    uint8_t received_segment_no;
    uint16_t end_packet;
} __attribute__((packed)) reject_packet_t; // Size 10

/**
 * @brief Error function
 *
 * @param msg Error message
 */
void error(const char *msg);

/**
 * @brief Timeout function, testing purposes only
 */
void timeout(void);

/**
 * @brief Verify if subscriber is in database and has paid or not.
 * 
 * @param verification_database read from verification_database.txt
 * @param subscriber_packet input subscriber packet
 * @return SUBSCRIBER_PACKET_TYPE 
 */
SUBSCRIBER_PACKET_TYPE verify_subscriber(
    verification_database_t verification_database[], uint8_t db_size, subscriber_packet_t *subscriber_packet);

// Validating that packet is correct
bool is_valid_data_packet(data_packet_t *packet);
bool is_valid_ack_packet(ack_packet_t *packet);
bool is_valid_reject_packet(reject_packet_t *packet);
bool is_valid_subscriber_packet(subscriber_packet_t *packet);

// Packet reset to default values. Clear payload array if it exists.
void reset_data_packet(data_packet_t *packet);
void reset_ack_packet(ack_packet_t *packet);
void reset_reject_packet(reject_packet_t *packet);
void reset_subscriber_packet(subscriber_packet_t *packet);

// Packet setters. Note: Should call reset() before setting values.
void update_data_packet(data_packet_t *packet, uint8_t client_id, uint8_t segment_no, uint8_t length, char *payload);
void update_ack_packet(ack_packet_t *packet, uint8_t client_id, uint8_t received_segment_no);
void update_reject_packet(reject_packet_t *packet, uint8_t client_id, REJECT_SUB_CODE sub_code, uint8_t received_segment_no);
void update_subscriber_packet(subscriber_packet_t *packet, uint8_t client_id, SUBSCRIBER_PACKET_TYPE packet_type, uint8_t segment_no, uint8_t technology, uint32_t src_sub_no);

// Data Packet Comparison Equal method: For checking duplicate packets
bool data_packet_equals(data_packet_t *packet1, data_packet_t *packet2);

// ToString Sizes, gathered from compiler warnings:
#define DATA_PACKET_STRING_SIZE 355
#define ACK_PACKET_STRING_SIZE 88
#define REJECT_PACKET_STRING_SIZE 103
/**
 * @brief Convert DATA packet to string. NOTE: MUST BE FOLLOWED BY A FREE() CALL TO PREVENT MEMORY LEAKS
 *
 * @param packet data_packet_t packet to convert
 * @return char* string representation of packet
 */
char *data_packet_to_string(data_packet_t *packet);
/**
 * @brief Convert ACK packet to string. NOTE: MUST BE FOLLOWED BY A FREE() CALL TO PREVENT MEMORY LEAKS
 *
 * @param packet ack_packet_t packet to convert
 * @return char* string representation of packet
 */
char *ack_packet_to_string(ack_packet_t *packet);
/**
 * @brief Convert REJECT packet to string. NOTE: MUST BE FOLLOWED BY A FREE() CALL TO PREVENT MEMORY LEAKS
 *
 * @param packet reject_packet_t packet to convert
 * @return char* string representation of packet
 */
char *reject_packet_to_string(reject_packet_t *packet);

// Print Verification Database:
void print_verification_database(verification_database_t verification_database[], uint8_t db_size);
// Print Subscriber Packet:
void print_subscriber_packet(subscriber_packet_t *subscriber_packet);

#endif