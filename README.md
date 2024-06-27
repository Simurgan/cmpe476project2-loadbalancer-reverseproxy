# Description

This is an educational distributed systems course project to better understand and practice concepts. The project consists of 5 components:

1. Watchdog: starts all other components and restarts them in case of their death.
2. Load balancer: distributes incoming requests into 2 reverse proxies according to clients' ID number.
3. Reverse proxy: distributes incoming requests into 3 servers randomly.
4. Server: takes incomig requests and replies as the squareroot of the number coming in the request.
5. Client: sends request to the system.

Load balancer forwards the incoming requests to the first reverse proxy if the client ID is odd and vice versa. The reverse proxies responds to incoming requests without forwarding to the servers if the requests are negative numbers. The watchdog creates the processes of all other components. It will start 1 load balancer, 2 reverse proxies, and 6 servers whose half is connected to 1 revrese proxy and the other half is connected to another. Watchdog will relaunch a process if that process dies. If watchdog receives SIGTSTP signal it will terminates all processes and itself at the end.

# Setup

Run make command in the src directory to compile the project:

```bash
make
```

You can also remove the output files by running:

```bash
make clean
```

# Usage

After compilation, run watchdog to activate the system:

```bash
./watchdog
```

In another terminal window, execute a client program with a client ID:

```bash
./client <client_id>
```

The client program will prompt you to enter a number.
You can follow the system logs in the watchdog window.
