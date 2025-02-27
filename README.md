# Network Message System

## About
* It contains workable HashMap realization based on lock mechanism, resizeble hashmap
* lock less realization does not work yet, need to add pointer first bit manipulation to protect pointers
* App should handle UDP messages, save it to the map and resend to TCP server
* Two udp threads can receive and save messages in a map
* Tcp thread handle specific messages from Udp threads

## Build Instructions
1. Clone the repository
2. run install.sh
3. make && make run
4. check main.cpp -> it can run all together
5. mannualy run:
* run TcpProcessor 50003
* run UdpProcessor 50001 50002 50003
* run MessageProcessor

## Techniques Used
- **POSIX Threads**: For multithreading.
- **POSIX Sockets**: For UDP and TCP communication.
- **Lock Free Containers**: For cuncurrent safe access to the message container.
- **Lock Containers**: For cuncurrent safe access to the message container.

## Why It Works
- **tests**: For tests were written separate application:
* checked information in console
* checked info received written in files in UDP and TCP processes separetly
* runned cuncurrent stress tests for container
- Non-blocking sockets ensure the system is optimized for quick responses (UDP handler works as expected)
* verifed with address, thread sanitizers
* verified with valgrind
