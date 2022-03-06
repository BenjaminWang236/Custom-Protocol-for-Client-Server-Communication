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

/**
 * @brief Read in the verification database from file.
 *
 * @param verification_database Pointer to the verification database.
 * @param filename string for the filename or path to the file.
 * @return uint8_t database size.
 */
uint8_t read_verification_database(verification_database_t verification_database[], char *filename)
{
    // Read in Verification Database from file's Variables:
    // char *filename = "verification_database.txt";
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    uint8_t db_size = 0;

    // Open file:
    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        error("Error opening file");
    }

    // Read the nuumber of database entries:
    getline(&line, &len, fp);
    db_size = atoi(line);
    memset(line, 0, len);

    // Check against maximum database size:
    if (db_size > VERIFICATION_DATABASE_SIZE)
        error("ERROR: Database size exceeds maximum size\n");

    // Read in verification database entries:
    for (int i = 0; i < db_size; i++)
    {
        // Read subscriber number (phone number):
        getline(&line, &len, fp);
        verification_database[i].src_sub_no = (uint32_t)atoi(line);
        memset(line, 0, len);

        // Read subscriber technology (2G - 5G):
        getline(&line, &len, fp);
        verification_database[i].technology = (SUBSCRIBER_TECHNOLOGY)atoi(line);
        memset(line, 0, len);

        // Read subscriber Paid status:
        getline(&line, &len, fp);
        verification_database[i].paid = (bool)atoi(line);
        memset(line, 0, len);
    }

    // Housekeeping:
    fclose(fp);
    if (line)
        free(line);

    return db_size;
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
    char *filename;

    // Checking if port number is provided
    if (argc == 1)
    {
        port = PORT;
        filename = "./input_files/verification_database.txt"; // Specificed by instruction
    }
    else if (argc == 2)
        port = atoi(argv[1]);
    else if (argc == 3)
    {
        port = atoi(argv[1]);
        filename = argv[2];
    }
    else
    {
        fprintf(stderr, "ERROR: no port & verification_database-filename provided\n");
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

    // Initializing verification database
    verification_database_t verification_database[VERIFICATION_DATABASE_SIZE] = {};
    uint8_t db_size = read_verification_database(verification_database, filename);

#ifdef PRINT_DATABASE
    // Print out Verification Database:
    print_verification_database(verification_database, db_size);
#endif

    // Custom protocol's Subscriber Packets:
    subscriber_packet_t subscriber_packet = {};
    uint8_t subscriber_packet_size = sizeof(subscriber_packet);
    SUBSCRIBER_PACKET_TYPE subscriber_status = DEFAULT_VALUE;

    // Server runs forever, I guess
    while (1)
    {
        // Receive Access Permission request Subscriber Packet:
        memset(&subscriber_packet, DEFAULT_VALUE, subscriber_packet_size);
        n = recvfrom(sock, &subscriber_packet, subscriber_packet_size,
                     0, (struct sockaddr *)&client, &clientlen);
        if (n < 0)
            error("ERROR: recvfrom");
        printf("\nReceived subscriber packet!\n");
        if (!is_valid_subscriber_packet(&subscriber_packet))
            error("ERROR: Invalid subscriber packet!\n");
#ifdef DEBUGGING
        else
            printf("Valid subscriber packet!\n");
        print_subscriber_packet(&subscriber_packet);
#endif

        // Verify subscriber by checking database:
        subscriber_status = verify_subscriber(
            verification_database, db_size, &subscriber_packet);
        printf("Responding with Subscriber status: 0x%04X\t", subscriber_status);
        if (subscriber_status == SUB_NOT_PAID)
            printf("%s\n", SUB_NOT_PAID_MSG);
        else if (subscriber_status == SUB_NOT_EXIST)
            printf("%s\n", SUB_NOT_EXIST_MSG);
        else if (subscriber_status == SUB_ACC_OK)
            printf("%s\n", SUB_ACC_OK_MSG);

        // Set responding subscriber packet's status:
        subscriber_packet.packet_type = subscriber_status;

        // Checking responding subscriber response integrity:
        if (!is_valid_subscriber_packet(&subscriber_packet))
            error("ERROR: Invalid response subscriber packet!\n");
#ifdef DEBUGGING
        else
            printf("Valid response subscriber packet!\n");
        print_subscriber_packet(&subscriber_packet);
#endif

        // Sending Subscriber status response back to Client
        n = sendto(sock, &subscriber_packet, subscriber_packet_size,
                   0, (const struct sockaddr *)&client, clientlen);
        if (n < 0)
            error("ERROR: sendto");
    }

    close(sock);
    return EXIT_SUCCESS;
}
