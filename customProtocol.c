/**
 * @file customProtocol.c
 * @author Benjamin Wang (bwang4@scu.edu, ID: 1179478)
 * @brief Client using customized protocol on top of UDP protocol for requesting
 *      identification from server for access permission to the cellular network.
 *      Header file implementing the CUSTOM PROTOCOL
 * @version 0.2
 * @date 2022-03-05
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "customProtocol.h"

/**
 * @brief Error function
 *
 * @param msg Error message
 */
void error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

SUBSCRIBER_PACKET_TYPE verify_subscriber(
    verification_database_t verification_database[], uint8_t db_size, subscriber_packet_t *subscriber_packet)
{
    for (int i = 0; i < db_size; i++)
    {
        if (verification_database[i].src_sub_no == subscriber_packet->src_sub_no &&
            verification_database[i].technology == subscriber_packet->technology)
        {
            if (verification_database[i].paid)
                return SUB_ACC_OK;
            else
                return SUB_NOT_PAID;
        }
    }
    return SUB_NOT_EXIST;
}

bool is_valid_subscriber_packet(subscriber_packet_t *packet)
{
    if (packet->start_packet != START_PACKET)
    {
        printf("Error: Invalid start packet 0x%04X\n", packet->start_packet);
        return false;
    }
    if (packet->packet_type < SUB_ACC_PER ||
        packet->packet_type > SUB_ACC_OK)
    {
        printf("Error: Invalid packet_type 0x%04X\n", packet->packet_type);
        return false;
    }
    if (packet->segment_no >= PACKET_GROUP_SIZE) // [0 - 4]
    {
        printf("Error: Invalid segment number %hu\n", packet->segment_no);
        return false;
    }
    if (packet->technology < SUB_2G ||
        packet->technology > SUB_5G)
    {
        printf("Error: Invalid technology %hu\n", packet->technology);
        return false;
    }
    if (packet->end_packet != END_PACKET)
    {
        printf("Error: Invalid end packet 0x%04X\n", packet->end_packet);
        return false;
    }
    return true;
}

void reset_subscriber_packet(subscriber_packet_t *packet)
{
    packet->start_packet = START_PACKET;
    packet->client_id = DEFAULT_VALUE;
    packet->packet_type = SUB_ACC_PER;
    packet->segment_no = DEFAULT_VALUE;
    packet->length = SUBSCRIBER_PAYLOAD_SIZE;
    packet->technology = DEFAULT_VALUE;
    packet->src_sub_no = DEFAULT_VALUE;
    packet->end_packet = END_PACKET;
}

void update_subscriber_packet(
    subscriber_packet_t *packet, uint8_t client_id, SUBSCRIBER_PACKET_TYPE packet_type,
    uint8_t segment_no, uint8_t technology, uint32_t src_sub_no)
{
    packet->client_id = client_id;
    packet->packet_type = packet_type;
    // packet->segment_no = segment_no % PACKET_GROUP_SIZE;
    packet->segment_no = segment_no;
    packet->technology = technology;
    packet->src_sub_no = src_sub_no;
}

void print_verification_database(verification_database_t verification_database[], uint8_t db_size)
{
    char phone[PHONE_NUMBER_SIZE + 1]; // +1 for '\0'
    printf("Verification Database:\nSubscriber Number\tTechnology\tPaid\n");
    for (int i = 0; i < db_size; i++)
    {
        memset(phone, DEFAULT_VALUE, PHONE_NUMBER_SIZE + 1);
        sprintf(phone, "%u", verification_database[i].src_sub_no);
        printf(
            "(%.3s) %.3s-%.4s\t\t%02u\t\t%i\n", phone, phone + 3, phone + 6,
            verification_database[i].technology, verification_database[i].paid);
    }
}
void print_subscriber_packet(subscriber_packet_t *subscriber_packet)
{
    char phone[PHONE_NUMBER_SIZE + 1]; // +1 for '\0'
    memset(phone, DEFAULT_VALUE, PHONE_NUMBER_SIZE + 1);
    sprintf(phone, "%u", subscriber_packet->src_sub_no);
    printf("\nSubscriber Packet:\n");
    printf(
        "client_id=\t%hu\npacket_type=\t0x%04X\nsegment_no=\t%hu\ntechnology=\t%hu\nsrc_sub_no=\t(%.3s) %.3s-%.4s\n",
        subscriber_packet->client_id,
        (uint16_t)subscriber_packet->packet_type,
        subscriber_packet->segment_no,
        subscriber_packet->technology,
        phone, phone + 3, phone + 6);
}
