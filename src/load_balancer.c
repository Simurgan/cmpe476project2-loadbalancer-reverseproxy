#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <signal.h>

#define MAX_BUFFER_SIZE 80 // Maximum buffer size for reading from the socket

int LB_PORT;
int RP_IDS[2];
int RP_PORTS[2];

void *handle_connection(void *);
char *forward_to_proxy(int, char *);
void sigterm_handler(int);

int main(int argc, char const *argv[])
{
    // Register SIGTERM signal handler
    signal(SIGTERM, sigterm_handler);

    // Extract load balancer port from command line arguments
    LB_PORT = atoi(argv[1]);

    // Extract reverse proxy IDs and ports from command line arguments
    for (int i = 0; i < 2; i++)
    {
        RP_IDS[i] = atoi(argv[2 + i]);
        RP_PORTS[i] = atoi(argv[4 + i]);
    }

    // Create a socket
    int lb_fd;
    if ((lb_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("\nSocket creation failed\n");
        exit(EXIT_FAILURE);
    }

    // Set socket options to reuse address and port
    int opt = 1;
    if (setsockopt(lb_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("\nSetsockopt failed\n");
        close(lb_fd);
        exit(EXIT_FAILURE);
    }

    // Define load balancer address
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(LB_PORT);

    // Bind the socket to the address
    if (bind(lb_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("\nPort binding failed\n");
        close(lb_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections from clients
    if (listen(lb_fd, 60) < 0)
    {
        perror("\nPort listening failed\n");
        close(lb_fd);
        exit(EXIT_FAILURE);
    }

    // Load balancer setup message
    printf("[LOAD BALANCER]: Load balancer has started. Listening on port %d.\n", LB_PORT);

    // Accept and handle incoming connections
    int socket_id;
    pthread_t thread_id;
    while (1)
    {
        // Accept incoming connection
        if ((socket_id = accept(lb_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("\nConnection accept failed\n");
            close(lb_fd);
            exit(EXIT_FAILURE);
        }

        // Allocate memory for socket_id and handle connection in a new thread
        int *socket_id_ptr = malloc(sizeof(int));
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

    // Close load balancer socket
    close(lb_fd);
    exit(EXIT_SUCCESS);
}

void *handle_connection(void *arg)
{
    // Detach the thread
    pthread_detach(pthread_self());

    // Get the socket_id from argument and free the allocated memory
    int socket_id = *(int *)arg;
    free(arg);
    char buffer[MAX_BUFFER_SIZE]; // Buffer to store incoming data

    // Read data from the client
    ssize_t byte_length = read(socket_id, buffer, MAX_BUFFER_SIZE);

    // Null-terminate the buffer
    buffer[byte_length] = '\0';

    // Copy buffer to avoid modifying the original buffer
    char buffer_copy[MAX_BUFFER_SIZE];
    strcpy(buffer_copy, buffer);

    // Extract client_id from the buffer
    int client_id = atoi(strtok(buffer_copy, " "));

    // Determine which proxy to forward the request to
    int proxy_index = 0;
    if (client_id % 2 == 0)
    {
        proxy_index = 1;
    }

    // Log the request forwarding
    printf("[LOAD BALANCER]: Request from Client #%d. Forwarding to Proxy #%d.\n", client_id, RP_IDS[proxy_index]);

    // Forward the request to the selected proxy
    char *result = forward_to_proxy(proxy_index, buffer);

    // Send the result back to the client
    send(socket_id, result, strlen(result), 0);
    free(result);

    // Close the client socket and exit the thread
    close(socket_id);
    pthread_exit(NULL);
}

char *forward_to_proxy(int proxy_index, char *buffer)
{
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
    serv_addr.sin_port = htons(RP_PORTS[proxy_index]);

    // Convert IP address to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        perror("\nInvalid address/ Address not supported \n");
        exit(EXIT_FAILURE);
    }

    // Connect to proxy
    if ((status = connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
    {
        perror("Connection failed\n");
        exit(EXIT_FAILURE);
    }

    // Send the request to the proxy
    send(client_fd, buffer, strlen(buffer), 0);

    // Allocate buffer for the response from the proxy
    char *buffer2 = (char *)malloc(MAX_BUFFER_SIZE * sizeof(char));

    // Read the response from the proxy
    ssize_t byte_length = read(client_fd, buffer2, MAX_BUFFER_SIZE);
    buffer2[byte_length] = '\0';

    // Close the proxy connection
    close(client_fd);
    return buffer2;
}

void sigterm_handler(int signo)
{
    // Handle SIGTERM signal
    printf("[LOAD BALANCER]: Received SIGTERM. Exiting...\n");
    exit(EXIT_SUCCESS);
}
