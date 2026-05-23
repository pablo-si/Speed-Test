# TCP Speedtest Client/Server

This repository contains a simple TCP-based speedtest implementation with two C programs:

- server.c: Listens for incoming connections, receives data for 30 seconds, and reports throughput.
- client.c: Connects to the server, sends data continuously for 30 seconds, and reports throughput.

---

## Prerequisites

- A C compiler (GCC).
- POSIX-compliant system (Linux or macOS).
- Network connectivity between client and server hosts , client can be connected to ethernet or Wi-Fi.

---

## Compilation

Open a terminal in the directory containing `server.c` and `client.c`, then run:

# Compile the server
gcc -Wall -O2 -o server server.c

# Compile the client
gcc -Wall -O2 -o client client.c

## Run

#Server
./server

#Client
./client <Server's Ip> <Port printed in server's terminal>