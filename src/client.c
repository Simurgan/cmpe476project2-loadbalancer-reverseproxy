#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>

#define LOAD_BALANCER_PORT 9090 // Port for the load balancer
#define MAX_BUFFER_SIZE 80      // Maximum buffer size for reading from the socket

int main(int argc, char const *argv[])
{
    // Extract client ID from command line arguments
    const char *client_id = argv[1];
    int status, client_fd;
    struct sockaddr_in serv_addr;

    // Create socket file descriptor
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("\nSocket creation error\n");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(LOAD_BALANCER_PORT); // Set port for load balancer

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        perror("\nInvalid address/ Address not supported \n");
        exit(EXIT_FAILURE);
    }

    // Connect to load balancer
    if ((status = connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
    {
        perror("Connection failed\n");
        exit(EXIT_FAILURE);
    }

    // Inform the user about the client ID and prompt for input
    printf("This is client #%s\nEnter a non-negative float: ", client_id);

    // Read user input
    char str[64];
    fgets(str, 64, stdin);

    // Remove newline character from input
    int new_line_index = strlen(str) - 1;
    if (str[new_line_index] == '\n')
    {
        str[new_line_index] = '\0';
    }

    // Prepare the string to send to the load balancer
    char sendstr[80];
    sendstr[0] = '\0';
    strcat(sendstr, client_id);
    strcat(sendstr, " ");
    strcat(sendstr, str);

    // Send the prepared string to the load balancer
    send(client_fd, sendstr, strlen(sendstr), 0);

    // Buffer to hold the response from the load balancer
    char buffer[MAX_BUFFER_SIZE];

    // Read the response from the load balancer
    ssize_t byte_length = read(client_fd, buffer, MAX_BUFFER_SIZE);
    buffer[byte_length] = '\0';

    // Print the result from the server
    printf("\tResult: %s\n", buffer);

    // Close the socket
    close(client_fd);

    // Exit the program successfully
    exit(EXIT_SUCCESS);
}
