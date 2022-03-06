/**
 * @file myclient.c
 * @author Benjamin Wang (bwang4@scu.edu, ID: 1179478)
 * @brief Client using customized protocol on top of UDP protocol for requesting
 *      identification from server for access permission to the cellular network.
 *      Implement the client that requests access permission to the cellular network.
 * @version 0.2
 * @date 2022-03-05
 *
 * @copyright Copyright (c) 2022
 * @source: https://www.linuxhowtos.org/C_C++/socket.htm
 * @source: https://www.geeksforgeeks.org/udp-server-client-implementation-c/
 *
 */

#include "customProtocol.h"

/**
 * @brief Main function (Driver code)
 *
 * @param argc number of arguments
 * @param argv arguments
 * @return int 0 if successful
 */
int main(int argc, char *argv[])
{
    // Local variables
    int sock, n, port, ack_timer_reset_count, seg_count;
    struct sockaddr_in server, recv_from;
    unsigned int length = sizeof(struct sockaddr_in);
    struct hostent *hp;
    bool response_received = false;
    struct timeval tv;
    uint8_t client_id, seg_no = 0, input_seg_no = 0, technology = 0;
    uint32_t src_sub_no = 0;

    // Default port number and hostname
    char *host = HOSTNAME;
    port = PORT;

    // Checking if usage is correct
    if (argc != 2)
    {
        printf("Usage: input_file\n");
        exit(EXIT_FAILURE);
    }

    // File IO variables
    char *filename = argv[1];
    FILE *fp;
    char *line = NULL;
    size_t len = 0;

    // Reading input file:
    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        error("Error opening file");
    }
    // Read the number of segments to send:
    getline(&line, &len, fp);
    seg_count = atoi(line);
    memset(line, 0, len);

    // Create socket with ACK Timer:
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        error("Error: socket");
    tv.tv_sec = ACK_TIMER_WAIT_TIME_MS / 1000;
    tv.tv_usec = (ACK_TIMER_WAIT_TIME_MS % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));

    // Filling server information
    server.sin_family = AF_INET;
    if ((hp = gethostbyname((const char *)host)) == 0)
        error("Error: Unknown host");
    bcopy((char *)hp->h_addr,
          (char *)&server.sin_addr,
          hp->h_length);
    server.sin_port = htons(port);

    // Custom protocol's Subscriber Packets:
    subscriber_packet_t subscriber_packet = {};
    uint8_t subscriber_packet_size = sizeof(subscriber_packet);
    SUBSCRIBER_PACKET_TYPE subscriber_status = DEFAULT_VALUE;

    // Loop through all the segments:
    for (seg_no = 0; seg_no < seg_count; seg_no++)
    {
        // Read Client ID:
        getline(&line, &len, fp);
        client_id = atoi(line);
        memset(line, 0, len);
        // Read Input Segment Number:
        getline(&line, &len, fp);
        input_seg_no = atoi(line) % PACKET_GROUP_SIZE;
        memset(line, 0, len);
        // Read subscriber technology:
        getline(&line, &len, fp);
        technology = atoi(line);
        memset(line, 0, len);
        // Read source subscriber number (phone number):
        getline(&line, &len, fp);
        src_sub_no = (uint32_t)atoi(line);
        memset(line, 0, len);

        response_received = false;
        // Retry up to 3 times, first time (0th) is the original packet
        for (ack_timer_reset_count = 0; ack_timer_reset_count <= ACK_TIMER_RETRY_COUNT && !response_received; ack_timer_reset_count++)
        {
#ifdef DEBUGGING
            printf("\n");
#endif
            if (ack_timer_reset_count == 0)
            {
                printf("Sending packet: %d\n", input_seg_no);
                reset_subscriber_packet(&subscriber_packet);
                update_subscriber_packet(&subscriber_packet, client_id, SUB_ACC_PER, input_seg_no, technology, src_sub_no);
                if (!is_valid_subscriber_packet(&subscriber_packet))
                    error("Error: Invalid subscriber packet\n");
#ifdef DEBUGGING
                else
                    printf("subscriber packet formatted okay\n");
                print_subscriber_packet(&subscriber_packet);
#endif
            }
            else
            {
                printf("Error:\tACK_TIMER timed out!\nRetrying attempt %d\n", ack_timer_reset_count);
            }

            // Send message to server
            n = sendto(sock, (const data_packet_t *)&subscriber_packet,
                       subscriber_packet_size, 0, (const struct sockaddr *)&server, length);
            if (n < 0)
                error("Error: Sendto");

            // Get response from server:
            reset_subscriber_packet(&subscriber_packet);
            n = recvfrom(sock, &subscriber_packet, subscriber_packet_size, 0, (struct sockaddr *)&recv_from, &length);
            if (n == -1 && errno == EAGAIN)
            {
                if (ack_timer_reset_count == ACK_TIMER_RETRY_COUNT)
                {
                    printf("Server does not respond\n");
                }
                response_received = false;
                continue;
            }
            else if (n < 0)
                error("Error: Recvfrom");
            response_received = true;
            subscriber_status = subscriber_packet.packet_type;

            // Print response from server:
            printf("Server responded with subscriber status: 0x%04X\t", subscriber_status);
            if (subscriber_status == SUB_NOT_PAID)
                printf("%s\n", SUB_NOT_PAID_MSG);
            else if (subscriber_status == SUB_NOT_EXIST)
                printf("%s\n", SUB_NOT_EXIST_MSG);
            else if (subscriber_status == SUB_ACC_OK)
                printf("%s\n", SUB_ACC_OK_MSG);
        }
        printf("\n");
    }

    // Housekeeping:
    fclose(fp);
    if (line)
        free(line);
    close(sock);
    return EXIT_SUCCESS;
}
