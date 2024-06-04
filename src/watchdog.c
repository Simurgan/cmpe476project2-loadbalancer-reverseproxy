#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>

#define LB_PORT "9090"                                                   // Load Balancer port
char *RP_IDS[] = {"1", "2"};                                             // Reverse Proxy IDs
char *RP_PORTS[] = {"9091", "9092"};                                     // Reverse Proxy ports
char *SERVER_IDS[] = {"1", "2", "3", "4", "5", "6"};                     // Server IDs
char *SERVER_PORTS[] = {"9093", "9094", "9095", "9096", "9097", "9098"}; // Server ports

pid_t LB_PID;         // Load Balancer process ID
pid_t RP_PIDS[2];     // Reverse Proxy process IDs
pid_t SERVER_PIDS[6]; // Server process IDs

int is_sigterm = 0; // Flag to indicate if SIGTERM signal is received

// Function declarations
pid_t create_load_balancer();
pid_t create_reverse_proxy(int);
pid_t create_server(int);
void sigchld_handler(int);
void sigtstp_handler(int);

int main()
{
    printf("[WATCHDOG]: Watchdog has started.\n");

    // Install SIGCHLD handler
    signal(SIGCHLD, sigchld_handler);

    // Install SIGTSTP handler
    signal(SIGTSTP, sigtstp_handler);

    // Create Load Balancer, Reverse Proxies, and Servers
    LB_PID = create_load_balancer();
    RP_PIDS[0] = create_reverse_proxy(0);
    RP_PIDS[1] = create_reverse_proxy(1);
    SERVER_PIDS[0] = create_server(0);
    SERVER_PIDS[1] = create_server(1);
    SERVER_PIDS[2] = create_server(2);
    SERVER_PIDS[3] = create_server(3);
    SERVER_PIDS[4] = create_server(4);
    SERVER_PIDS[5] = create_server(5);

    // Infinite loop to keep the program running
    while (1)
        ;

    return 0;
}

pid_t create_load_balancer()
{
    printf("[WATCHDOG]: Creating Load Balancer.\n");
    pid_t pid = fork(); // Fork a new process
    if (pid == 0)
    {
        // Child process: execute load balancer
        char *argv[] = {"./load_balancer", LB_PORT, RP_IDS[0], RP_IDS[1], RP_PORTS[0], RP_PORTS[1], NULL};
        execv("./load_balancer", argv);
    }
    return pid; // Return the process ID of the load balancer
}

pid_t create_reverse_proxy(int rp_index)
{
    printf("[WATCHDOG]: Creating Reverse Proxy #%s.\n", RP_IDS[rp_index]);
    pid_t pid = fork(); // Fork a new process
    if (pid == 0)
    {
        // Child process: execute reverse proxy
        char *argv[] = {"./reverse_proxy", RP_IDS[rp_index], RP_PORTS[rp_index], SERVER_IDS[3 * rp_index + 0], SERVER_IDS[3 * rp_index + 1], SERVER_IDS[3 * rp_index + 2], SERVER_PORTS[3 * rp_index + 0], SERVER_PORTS[3 * rp_index + 1], SERVER_PORTS[3 * rp_index + 2], NULL};
        execv("./reverse_proxy", argv);
    }
    return pid; // Return the process ID of the reverse proxy
}

pid_t create_server(int server_index)
{
    printf("[WATCHDOG]: Creating Server #%s.\n", SERVER_IDS[server_index]);
    pid_t pid = fork(); // Fork a new process
    if (pid == 0)
    {
        // Child process: execute server
        char *argv[] = {"./server", SERVER_IDS[server_index], SERVER_PORTS[server_index], NULL};
        execv("./server", argv);
    }
    return pid; // Return the process ID of the server
}

void sigchld_handler(int signo)
{
    if (!is_sigterm) // if the stuation is not an intended SIGTSTP
    {
        int status;
        pid_t failed_pid;

        // Get the process ID of the failed process
        failed_pid = waitpid(-1, &status, 0);

        if (failed_pid > 0)
        {
            // Relaunch Load Balancer if it failed
            if (failed_pid == LB_PID)
            {
                printf("[WATCHDOG]: Load Balancer failed. Relaunching...\n");
                LB_PID = create_load_balancer();
                return;
            }

            // Relaunch Reverse Proxy if it failed
            for (int i = 0; i < 2; i++)
            {
                if (failed_pid == RP_PIDS[i])
                {
                    printf("[WATCHDOG]: Reverse Proxy #%s failed. Relaunching...\n", RP_IDS[i]);
                    RP_PIDS[i] = create_reverse_proxy(i);
                    return;
                }
            }

            // Relaunch Server if it failed
            for (int i = 0; i < 6; i++)
            {
                if (failed_pid == SERVER_PIDS[i])
                {
                    printf("[WATCHDOG]: Server #%s failed. Relaunching...\n", SERVER_IDS[i]);
                    SERVER_PIDS[i] = create_server(i);
                    return;
                }
            }
        }
        else
        {
            perror("waitpid"); // Handle waitpid errors
        }
    }
}

void sigtstp_handler(int signo)
{
    is_sigterm = 1;
    printf("[WATCHDOG]: Received SIGTSTP. Terminating all processes...\n");

    // Terminate all server processes
    for (int i = 0; i < 6; i++)
    {
        kill(SERVER_PIDS[i], SIGTERM);
    }
    for (int i = 0; i < 6; i++)
    {
        wait(NULL); // Wait for all server processes to terminate
    }

    // Terminate all reverse proxy processes
    for (int i = 0; i < 2; i++)
    {
        kill(RP_PIDS[i], SIGTERM);
    }
    for (int i = 0; i < 2; i++)
    {
        wait(NULL); // Wait for all reverse proxy processes to terminate
    }

    // Terminate the load balancer process
    kill(LB_PID, SIGTERM);
    wait(NULL); // Wait for the load balancer process to terminate

    printf("[WATCHDOG]: All processes terminated. Exiting...\n");
    exit(EXIT_SUCCESS);
}
