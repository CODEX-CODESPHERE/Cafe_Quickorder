# Cafe QuickOrder — Networked C++ Terminal Project

A terminal-only C++17 client/server application. It contains **two independent servers**:
- **Restaurant Server** (`restaurant_server`): owns the menu, orders, cancellations, item availability and sales report.
- **Student Server** (`student_server`): registers/authenticates students and forwards their menu/order requests to the Restaurant Server over TCP networking.

There are also two terminal clients:
- `student_client`: student registration, login, menu, placing orders, viewing/cancelling their own orders.
- `restaurant_admin`: restaurant operator menu, all orders, sales report and item availability.

No GUI, web framework, external database, or third-party library is used. Data is persisted in text files beside the server executables.

## Requirements
- macOS or Linux
- C++17 compiler (`clang++` on macOS or `g++` on Linux)
- Both servers must be able to reach each other over TCP.

## Build (macOS / Linux)
Open Terminal in this project folder:

```bash
chmod +x build.sh
./build.sh
```

This creates four executables in `bin/`.

## Run locally (3 Terminal windows)
**Terminal 1 — restaurant server**
```bash
./bin/restaurant_server 5001
```

**Terminal 2 — student server** (connects to restaurant server)
```bash
./bin/student_server 5002 127.0.0.1 5001
```

**Terminal 3 — student client**
```bash
./bin/student_client 127.0.0.1 5002
```

To operate the restaurant/admin side, open a fourth terminal:
```bash
./bin/restaurant_admin 127.0.0.1 5001
```

## Run on two different computers
1. Start `restaurant_server` on the restaurant computer:
   `./bin/restaurant_server 5001`
2. Find that computer's LAN IP address. Ensure TCP port `5001` is allowed through its firewall.
3. Start the student server on the student-side computer and give it the restaurant computer's IP:
   `./bin/student_server 5002 RESTAURANT_LAN_IP 5001`
4. Students connect to the student server computer's IP:
   `./bin/student_client STUDENT_SERVER_LAN_IP 5002`
5. Restaurant staff connect admin terminal to restaurant server:
   `./bin/restaurant_admin RESTAURANT_LAN_IP 5001`

Both machines must be on a network that permits direct TCP connections. Do not expose these ports directly to the public internet; use a trusted LAN or VPN.

## Verify the project
After building, run the automated end-to-end test:

```bash
python3 test_network.py
```

The test starts both servers on temporary ports and checks registration, login, menu retrieval, network order placement, bill total, student order privacy, cancellation, sales report, and persistence after a server restart. It uses temporary data files and does not overwrite your real project data.

## Features
- Student registration with unique student ID and login.
- Password values are hashed before being stored (standard-library `std::hash`; suitable only for a classroom demo, not production authentication).
- Restaurant menu with item IDs, prices and availability.
- Multi-item order placement and automatic total calculation.
- Each order is linked to the authenticated student.
- Students can view only their own orders and cancel their own active orders.
- Restaurant admin can view every order, sales totals and update item availability.
- Concurrent client handling using one thread per TCP connection.
- Restaurant orders and student accounts persist after server restart.

## How the two-server network works

```text
Student terminal client
        |
        | TCP (port 5002)
        v
Student Server  -------- TCP (port 5001) --------> Restaurant Server
  - register/login                                  - menu and prices
  - student identity                                - order records
  - student order history                           - cancellation status
                                                     - sales report

Restaurant admin terminal ------------------------> Restaurant Server
```

The Student Server is the gateway for student accounts and forwards allowed order operations to the Restaurant Server. Restaurant administration connects directly to the Restaurant Server.

## Network protocol
TCP, newline-delimited text messages. Student clients connect only to the Student Server. The Student Server forwards restaurant operations to the Restaurant Server.

## Data files
- `students_data.txt` — created by the Student Server.
- `restaurant_data.txt` — created by the Restaurant Server.

Keep these files with their respective server executable/current working directory. Back them up before deleting or moving the project.

## Important limitations
This is a functional educational LAN prototype, not a production payment or security system. It has no payment gateway, TLS encryption, password salting/slow hashing, database server, password reset, or administrator authorization. Use trusted networks only. `std::hash` is not a secure password-storage algorithm. Add TLS and a dedicated password hashing library (Argon2/bcrypt/scrypt) before real deployment.
