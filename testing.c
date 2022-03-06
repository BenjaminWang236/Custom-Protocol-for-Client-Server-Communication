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
    char str[PHONE_NUMBER_SIZE + 1]; // +1 for '\0'
    uint32_t num = 4085546805;
    // printf("%u\n", num);
    sprintf(str, "%u", num);
    printf("Stringified, formatted:\t(%.3s) %.3s-%.4s\n", str, str + 3, str + 6);

    return EXIT_SUCCESS;
}
