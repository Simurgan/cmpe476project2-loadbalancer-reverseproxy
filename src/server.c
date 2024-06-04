#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <signal.h>

#define MAX_BUFFER_SIZE 80 // Maximum buffer size for reading from the socket

int SERVER_ID;
int SERVER_PORT;
void *handle_connection(void *);
void sigterm_handler(int);

int main(int argc, char const *argv[])
{
    // Register SIGTERM signal handler
    signal(SIGTERM, sigterm_handler);

    // Extract server ID and port from command line arguments
    SERVER_ID = atoi(argv[1]);
    SERVER_PORT = atoi(argv[2]);
    int server_fd;

    // Create a socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("\nSocket creation failed\n");
        exit(EXIT_FAILURE);
    }

    // Set socket options to reuse address and port
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("\nSetsockopt failed\n");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Define server address
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);

    // Bind the socket to the address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("\nPort binding failed\n");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 10) < 0)
    {
        perror("\nPort listening failed\n");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Server setup message
    printf("[SERVER #%d]: Server has started. Listening on port %d.\n", SERVER_ID, SERVER_PORT);

    // Accept and handle incoming connections
    int socket_id;
    pthread_t thread_id;
    while (1)
    {
        // Accept incoming connection
        if ((socket_id = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("\nConnection accept failed\n");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        // Allocate memory for socket_id and handle connection in a new thread
        int *socket_id_ptr = (int *)malloc(sizeof(int));
        *socket_id_ptr = socket_id;

        // Create a new thread for each client connection
        if (pthread_create(&thread_id, NULL, handle_connection, (void *)socket_id_ptr) != 0)
        {
            perror("\nPthread_create failed\n");
            close(socket_id);
            free(socket_id_ptr);
            continue;
        }
    }

    // Close server socket
    close(server_fd);
    exit(EXIT_SUCCESS);
}

void *handle_connection(void *arg)
{
    // Detach the thread from its parent thread
    pthread_detach(pthread_self());

    // Get the socket_id from argument and free the allocated memory
    int socket_id = *(int *)arg;
    free(arg);
    char buffer[MAX_BUFFER_SIZE]; // Buffer to store incoming data

    // Read data from the reverse proxy
    ssize_t byte_length = read(socket_id, buffer, MAX_BUFFER_SIZE);

    // Null-terminate the buffer
    buffer[byte_length] = '\0';

    // Copy buffer to avoid modifying the original buffer
    char buffer_copy[MAX_BUFFER_SIZE];
    strcpy(buffer_copy, buffer);

    // Extract client_id and req_num from the buffer
    int client_id = atoi(strtok(buffer_copy, " "));
    float req_num = atof(strtok(NULL, " "));

    // Print received value and calculated square root
    printf("[SERVER #%d]: Received the value %.2f from Client #%d. Returning %.2f\n", SERVER_ID, req_num, client_id, sqrt(req_num));

    // Prepare response
    char response[MAX_BUFFER_SIZE];
    sprintf(response, "%.2f", sqrt(req_num));

    // Send response back to the client
    send(socket_id, response, strlen(response), 0);

    // Close the client socket and exit the thread
    close(socket_id);
    pthread_exit(NULL);
}

void sigterm_handler(int signo)
{
    // Handle SIGTERM signal
    printf("[SERVER #%d]: Received SIGTERM. Exiting...\n", SERVER_ID);
    exit(EXIT_SUCCESS);
}
