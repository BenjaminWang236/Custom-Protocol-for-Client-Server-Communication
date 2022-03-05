/**
 * @file myclient.c
 * @author Benjamin Wang (bwang4@scu.edu, ID: 1179478)
 * @brief Client using customized protocol on top of UDP protocol for sending
 * information to the server. Client side implementation of UDP client-server.
 * @version 0.1
 * @date 2022-03-02
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
    char buffer[PACKET_DATA_PAYLOAD_SIZE];
    uint8_t peek_buff[1];
    bool response_received = false;
    struct timeval tv;
    uint8_t client_id, seg_no = 0, buffLen, input_seg_no;

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

    // Create socket:
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

    // Custom Protocol Packets:
    data_packet_t data_packet = {};
    size_t data_packet_size = sizeof(data_packet);
    ack_packet_t ack_packet = {};
    size_t ack_packet_size = sizeof(ack_packet);
    reject_packet_t reject_packet = {};
    size_t reject_packet_size = sizeof(reject_packet);

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
        // Read Payload:
        getline(&line, &len, fp);
        memset(buffer, 0, PACKET_DATA_PAYLOAD_SIZE);
        strcpy(buffer, line);
        buffer[strcspn(buffer, "\n")] = 0;
        buffLen = strlen(buffer);
        memset(line, 0, len);

        // Error handling special cases:
        if (client_id == ERROR_SPECIAL_CLIENT)
        {
            if (strcmp(buffer, ERR_OOS_MSG) == 0)
            {
                printf("Error handling: Case 1: Out of Sequence segment number\n");
            }
            else if (strcmp(buffer, ERR_LEN_MSG) == 0)
            {
                printf("Error handling: Case 2: Length Mismatch\n");
            }
            else if (strcmp(buffer, ERR_END_MSG) == 0)
            {
                printf("Error handling: Case 3: End of Packet Missing\n");
            }
            else if (strcmp(buffer, ERR_DUP_MSG) == 0)
            {
                printf("Error handling: Case 4: Duplicate Packet check done by comparing sequence number as specified in instruction\n");
            }
        }

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
                reset_data_packet(&data_packet);
                update_data_packet(&data_packet, client_id, input_seg_no, buffLen, buffer);
                if (!is_valid_data_packet(&data_packet))
                    error("Error: Invalid data packet\n");
#ifdef DEBUGGING
                else
                    printf("Data packet formatted okay\n");
                char *data_str = data_packet_to_string(&data_packet);
                printf("Sending DATA packet: %s", data_str);
                free(data_str);
#endif
            }
            else
            {
                printf("Error:\tACK_TIMER timed out!\nRetrying attempt %d\n", ack_timer_reset_count);
            }

            // Error handling cases: Modifications to correct Data packet:
            if (client_id == ERROR_SPECIAL_CLIENT)
            {
                if (strcmp(buffer, ERR_LEN_MSG) == 0)
                {
                    // printf("Error handling: Case 2: Length Mismatch\n");
                    data_packet.length = (data_packet.length + 1) % PACKET_DATA_PAYLOAD_SIZE;
                }
                if (strcmp(buffer, ERR_END_MSG) == 0)
                {
                    // printf("Error handling: Case 3: End of Packet Missing\n");
                    data_packet.end_packet = 0xFFF0;
                }
            }

            // Send message to server
            n = sendto(sock, (const data_packet_t *)&data_packet,
                       data_packet_size, 0, (const struct sockaddr *)&server, length);
            if (n < 0)
                error("Error: Sendto");

            // Peek the response packet: Non-Blocking-ly
            // NOTE: Peek only works on non-UNIX systems, for example doesn't work on WSL-Ubuntu
            bzero(peek_buff, sizeof(peek_buff));
            n = recvfrom(sock, peek_buff, sizeof(peek_buff), MSG_PEEK | MSG_TRUNC, (struct sockaddr *)&recv_from, &length);
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
                error("Error: peek Recvfrom");
#ifdef DEBUGGING
            printf("Peeked packet size: %d\n", n);
#endif

            // Processing the Response from Server:
            if (n == ack_packet_size)
            {
                memset(&ack_packet, 0, ack_packet_size);
                n = recvfrom(sock, &ack_packet, ack_packet_size, 0, (struct sockaddr *)&recv_from, &length);
                if (n < 0)
                    error("Error: recvfrom");
                response_received = true;

                if (ack_packet.received_segment_no == input_seg_no)
                    printf("Received matching ACK segment number %d\n", input_seg_no);
                else
                    printf("Received MISMATCHING ACK segment number %d\n", ack_packet.received_segment_no);
                if (is_valid_ack_packet(&ack_packet))
                    printf("Received valid ACK packet.\n");
#ifdef DEBUGGING
                else
                    printf("Received invalid ACK packet.\n");
                char *ack_str = ack_packet_to_string(&ack_packet);
                printf("Received ACK packet: %s", ack_str);
                free(ack_str);
#endif
                break;
            }
            else if (n == reject_packet_size)
            {
                memset(&reject_packet, 0, reject_packet_size);
                n = recvfrom(sock, &reject_packet, reject_packet_size, 0, (struct sockaddr *)&recv_from, &length);
                if (n < 0)
                    error("Error: recvfrom");
                response_received = true;

                // printf("Raw reject packet: 0x%40X\n", reject_packet);
                if (is_valid_reject_packet(&reject_packet))
                    printf("Received valid REJ packet\n");
#ifdef DEBUGGING
                else
                    printf("ERROR: REJ packet formatted improperly!\n");
                char *rej_str = reject_packet_to_string(&reject_packet);
                printf("Received REJECT packet: %s\n", rej_str);
                free(rej_str);

#endif
                printf("Reject sub code: 0x%04X\n", reject_packet.sub_code);
                break;
                // If rejected, move on to next packet to send
            }
            else
            {
                // This resonse is not an ACK or REJECT packet, so should not count as received properly
                char *errString = "";
                sprintf(errString, "Error: Invalid packet size/type received: %d\n", n);
                error(errString);
            }
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
