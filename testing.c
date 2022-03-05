/**
 * @file generateTestInputs.c
 * @author Benjamin Wang (bwang4@scu.edu, ID: 1179478)
 * @brief Client using customized protocol on top of UDP protocol for requesting
 *      identification from server for access permission to the cellular network.
 *      Generate the tests, and also debugging
 * @version 0.2
 * @date 2022-03-05
 *
 * @copyright Copyright (c) 2022
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
    // Checking if usage is correct
    if (argc != 2)
    {
        printf("Usage: input_file\n");
        exit(EXIT_FAILURE);
    }
    char *filename = argv[1];

    // File IO variables
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        error("Error opening file");
    }
    while ((read = getline(&line, &len, fp)) != -1)
    {
        printf("Retrieved line of length %zu:\n", read);
        printf("%s", line);
    }
    fclose(fp);
    if (line)
        free(line);

    printf("Test: Rej Dup Packet:\t0x%04X\n", REJECT_DUPLICATE_PACKET);
    printf("Test: hostname: %s\n", HOSTNAME);
    printf("Test: data_packet_t size: %i\n", (int)sizeof(data_packet_t));
    printf("Test: ack_packet_t size: %i\n", (int)sizeof(ack_packet_t));
    printf("Test: reject_packet_t size: %i\n", (int)sizeof(reject_packet_t));
    // timeout();
    char *hello = "Hello World!";
    data_packet_t data_packet = {};
    reset_data_packet(&data_packet);
    update_data_packet(&data_packet, 255, 5, strlen(hello), hello);
    if ((is_valid_data_packet(&data_packet)))
        printf("Test: data_packet is valid\n");
    else
        printf("Test: data_packet is invalid\n");
    // printf("payload: %s\n", data_packet.payload);
    printf("data_packet_to_string: %s", data_packet_to_string(&data_packet));
    printf("payload length: strlen: %lu\tsizeof: %lu\n", strlen(data_packet.payload), sizeof(data_packet.payload));

    return EXIT_SUCCESS;
}
