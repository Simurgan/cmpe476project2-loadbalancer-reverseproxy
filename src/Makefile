all: watchdog load_balancer reverse_proxy server client

watchdog: watchdog.c
	gcc watchdog.c -o watchdog

load_balancer: load_balancer.c
	gcc load_balancer.c -o load_balancer

reverse_proxy: reverse_proxy.c
	gcc reverse_proxy.c -o reverse_proxy

server: server.c
	gcc server.c -o server -lm

client: client.c
	gcc client.c -o client

clean:
	rm -f watchdog load_balancer reverse_proxy server client
