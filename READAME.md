# C Multi-Threaded Global Chat Application

A concurrent client-server chat application implemented in **C**, featuring user authentication (Sign-up/Login), persistent storage using **MongoDB**, and real-time multi-client message broadcasting via **POSIX Threads (pthreads)** and TCP/IP sockets.

## Features

- **User Authentication:** Secure sign-up and login mechanism with password validation and 3-attempt limit.
- **Concurrent Client Handling:** The server uses `pthreads` and mutexes (`pthread_mutex_t`) to safely manage multiple simultaneous client connections.
- **Database Integration:** User credentials are securely stored and retrieved using the MongoDB C Driver (`libmongoc`).
- **Real-Time Broadcasting:** Messages sent by any connected client are dynamically broadcasted to all other active chat participants.
- **Signal Handling:** Graceful server shutdown handling using `sigaction` and `SIGINT`.

## Tech Stack

- **Language:** C (GNU99 / POSIX)
- **Networking:** POSIX Sockets (TCP/IP)
- **Concurrency:** POSIX Threads, Mutex locks
- **Database:** MongoDB (via `libmongoc`)
- **Build System:** Make / GCC

---

## Prerequisites & Dependencies

To compile and run this project, you need a UNIX/Linux environment with **GCC**, **Make**, and the **MongoDB C Driver** installed.

1. **Install MongoDB C Driver (`libmongoc`):**
   - On Ubuntu/Debian:
```bash
sudo apt-get install libmongoc-dev libbson-dev
Ensure a local MongoDB instance is running:

Bash
mongod --dbpath /path/to/your/db
Compilation & Building
A Makefile is provided to compile both the server and client executables automatically. Simply run:

Bash
make
This will generate two binaries: server and client.

Usage / Running the Application
1. Start the Server
Run the server by specifying a port number:

Bash
./server <PORT>
# Example:
./server 8080
2. Connect a Client
In a separate terminal window(s), run the client by providing the server's IP address and port:

Bash
./client <IP_ADDRESS> <PORT>
# Example:
./client 127.0.0.1 8080
Once connected, follow the interactive terminal prompt to Sign-up or Login, and start chatting in real time!

Project Structure
server.c — Handles socket binding, incoming connections, multi-threading, client broadcasting, and MongoDB queries.

client.c — Handles user interface (GUI-like terminal prompts), authentication steps, and concurrent sending/receiving threads.

Makefile — Automated build script with pkg-config integration for MongoDB libraries.

Author
Christopoulos Nikolas