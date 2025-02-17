# Network Message System

## Build Instructions
1. Clone the repository.

## Techniques Used
- **POSIX Threads**: For multithreading.
- **POSIX Sockets**: For UDP and TCP communication.
- **Lock Free Containers**: For cuncurrent safe access to the message container.
- **Non-blocking Sockets**: For quick message handling.

## Why It Works
- The system uses separate threads for receiving UDP messages and sending TCP messages, ensuring responsiveness.
- The message container is thread-safe, allowing concurrent access.
- Non-blocking sockets ensure the system is optimized for quick responses.
