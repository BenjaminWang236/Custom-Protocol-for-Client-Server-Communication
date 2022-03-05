/**
 * @file myserver.c
 * @author Benjamin Wang (bwang4@scu.edu, ID: 1179478)
 * @brief Client using customized protocol on top of UDP protocol for requesting
 *      identification from server for access permission to the cellular network.
 *      Implement the server and the access permission identification.
 * @version 0.2
 * @date 2022-03-05
 *
 * @copyright Copyright (c) 2022
 * @source: https://www.linuxhowtos.org/C_C++/socket.htm
 * @source: https://www.geeksforgeeks.org/udp-server-client-implementation-c/
 *
 */

#include "customProtocol.h"

uint16_t validate_client_data_packet(bool *packet_seq_started, uint8_t *packet_seq_num, data_packet_t *curr_data, data_packet_t *prev_data)
{
    // if (*packet_seq_started && data_packet_equals(curr_data, prev_data))
    if (*packet_seq_started && curr_data->segment_no == prev_data->segment_no)
    {
        printf("ERROR: Duplicate packet received!\n");
        return REJECT_DUPLICATE_PACKET;
    }
    if (*packet_seq_started && curr_data->segment_no != (*packet_seq_num + 1))
    {
#ifdef DEBUGGING
        printf("Error: Expected sequence number %d\n", *packet_seq_num + 1);
#endif
        printf("ERROR: Out of order packet!\n");
        return REJECT_OUT_OF_SEQUENCE;
    }
    else if (!(*packet_seq_started) && curr_data->segment_no != 0)
    {
#ifdef DEBUGGING
        printf("Error: Expected sequence number %d\n", 0);
#endif
        printf("ERROR: Out of order packet!\n");
        return REJECT_OUT_OF_SEQUENCE;
    }
    if (strlen(curr_data->payload) != curr_data->length)
    {
        printf("ERROR: Length Mismatch!\n");
        return REJECT_LENGTH_MISMATCH;
    }
    if (curr_data->end_packet != END_PACKET)
    {
        printf("ERROR: End of Packet Missing!\n");
        return REJECT_END_OF_PACKET_MISSING;
    }

    return PACKET_OK;
}

/**
 * @brief Main function (Driver code)
 *
 * @param argc number of arguments
 * @param argv arguments
 * @return int 0 if successful
 */
int main(int argc, char *argv[])
{
    // Define variables
    int sock, length, n, port;
    socklen_t clientlen;
    struct sockaddr_in server, client;
    // char buf[MAXLINE];
    // char *hello = "Hello from server";

    // Checking if port number is provided
    if (argc == 1)
        port = PORT;
    else if (argc == 2)
        port = atoi(argv[1]);
    else
    {
        fprintf(stderr, "ERROR: no port provided\n");
        exit(EXIT_FAILURE);
    }

    // Creating socket file descriptor
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        error("ERROR: Opening socket");

    // Filling server information
    length = sizeof(server);
    bzero(&server, length); // memset(&servaddr, 0, length);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    // Bind the socket with the server address
    if (bind(sock, (struct sockaddr *)&server, length) < 0)
        error("ERROR: binding");

    // Initializing client
    clientlen = sizeof(client);
    bzero(&client, clientlen);

    // Custom protocol's Packets:
    data_packet_t data_packet = {};
    size_t data_packet_size = sizeof(data_packet);
    ack_packet_t ack_packet = {};
    size_t ack_packet_size = sizeof(ack_packet);
    reject_packet_t reject_packet = {};
    size_t reject_packet_size = sizeof(reject_packet);
    data_packet_t prev_data_packet = {}; // For checking duplicates

    // Packet Tracker:
    bool packet_seq_started = false;
    uint8_t packet_seq_num = 0;
    uint16_t packet_status = 1; // 0 if OK, else REJECT sub code

    // Server runs forever, I guess
    while (1)
    {
        memset(&data_packet, 0, data_packet_size);
        n = recvfrom(sock, &data_packet, data_packet_size, 0, (struct sockaddr *)&client, &clientlen);
        if (n < 0)
            error("ERROR: recvfrom");
        printf("\nReceived packet!\n");
#ifdef DEBUGGING
        char *data_str = data_packet_to_string(&data_packet);
        printf("Received data packet: %s", data_str);
        free(data_str);
#endif

        // ACK or REJ logic here: All 4 Server Reject Sub-Code cases are handled here
        packet_status = validate_client_data_packet(&packet_seq_started, &packet_seq_num, &data_packet, &prev_data_packet);

        if (packet_status == PACKET_OK)
        {
#ifdef DEBUGGING
            printf("Packet OK!\n");
#endif
            // Tracking the packet sequence: Since packet's valid
            if (!packet_seq_started)
                packet_seq_started = true;
            else
                packet_seq_num = data_packet.segment_no;

            // Saving the current packet as next "prev_data_packet" for duplicate checking
            memset(&prev_data_packet, 0, data_packet_size);
            memcpy(&prev_data_packet, &data_packet, data_packet_size);

            // ACK packet response:
            reset_ack_packet(&ack_packet);
            update_ack_packet(&ack_packet, data_packet.client_id, packet_seq_num);
            if (!is_valid_ack_packet(&ack_packet))
                error("ERROR: Invalid ACK packet");
#ifdef DEBUGGING
            else
                printf("ACK packet formatted properly!\n");
#endif
            // Sending ACK response back to Client
            n = sendto(sock, &ack_packet, ack_packet_size,
                       0, (const struct sockaddr *)&client, clientlen);
        }
        else
        {
#ifdef DEBUGGING
            printf("Packet Rejected with REJ sub code 0x%04X\n", packet_status);
#endif
            // REJ packet response:
            reset_reject_packet(&reject_packet);
            update_reject_packet(&reject_packet, data_packet.client_id, packet_status, packet_seq_num);
            if (!is_valid_reject_packet(&reject_packet))
                error("ERROR: REJ packet formatted improperly!\n");
#ifdef DEBUGGING
            else
                printf("REJ packet formatted properly!\n");
            char *rej_str = reject_packet_to_string(&reject_packet);
            printf("Sending response REJ packet: %s\n", rej_str);
            free(rej_str);
#endif
            // Sending ACK response back to Client
            n = sendto(sock, &reject_packet, reject_packet_size,
                       0, (const struct sockaddr *)&client, clientlen);
        }
        if (n < 0)
            error("ERROR: sendto");

        // Resetting the packet status after max of 5 packets
        if (packet_seq_started && packet_seq_num >= (PACKET_GROUP_SIZE - 1))
        {
#ifdef DEBUGGING
            printf("SERVER RESETTING SEQ NUM AND SEQ STARTED\n");
#endif
            packet_seq_num = 0;
            packet_seq_started = false;
        }
#ifdef DEBUGGING
        printf("\n\n");
#endif
    }

    close(sock);
    return EXIT_SUCCESS;
}
