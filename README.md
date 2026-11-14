# Cafe QuickOrder — Beginner-Friendly C++ Networking Project

Cafe QuickOrder is a **terminal-only C++17 project** for a first programming/networking project. It uses simple structures, vectors, functions, file handling, TCP sockets, and threads. **It does not use a priority queue.**

## What is included?

- **Restaurant Server** (restaurant_server.cpp): keeps the food menu and orders, calculates bills, handles cancellations, and prepares a sales report.
- **Student Server** (student_server.cpp): registers students, checks login details, and forwards student requests to the Restaurant Server over TCP.
- **Student Client** (student_client.cpp): lets a student register, log in, view the menu, place an order, see their orders, and cancel an order.
- **Restaurant Admin** (restaurant_admin.cpp): lets the restaurant operator view the menu, see all orders, view sales, and change whether a food item is available.
- **net.hpp**: small helper functions for sending and receiving text messages over TCP.
- **Text files**: keep student accounts, menu availability, and orders between program runs.

No GUI, web framework, external database, or third-party C++ library is required.

## Requirements

- macOS or Linux
- A C++17 compiler (clang++ on macOS or g++ on Linux)
- Python 3 only if you want to run the automated test

## Easiest way to run both Student and Restaurant

Open Terminal in the project folder and run:

~~~bash
chmod +x run_local.sh
./run_local.sh
~~~

The script builds the programs (if needed), starts both servers, and displays a small terminal menu:

~~~text
1. Open Student Terminal
2. Open Restaurant Admin Terminal
0. Stop both servers and exit
~~~

Choose 1 to use the student side or 2 to use the restaurant side. Both servers stay running while you return to this menu. Choose 0 to stop the servers.

The demo restaurant admin PIN is **1234**. It is checked by the Restaurant Server. For a classroom exercise, you can change ADMIN_PIN near the top of restaurant_server.cpp and rebuild.

## Manual run in separate Terminal windows

Build the programs first:

~~~bash
sh ./build.sh
~~~

Then open separate terminal windows and run each command from the project folder.

**Window 1 — Restaurant Server**

~~~bash
./bin/restaurant_server 5001
~~~

**Window 2 — Student Server**

~~~bash
./bin/student_server 5002 127.0.0.1 5001
~~~

**Window 3 — Student Client**

~~~bash
./bin/student_client 127.0.0.1 5002
~~~

**Window 4 — Restaurant Admin**

~~~bash
./bin/restaurant_admin 127.0.0.1 5001
~~~

Register a student first, then log in. The student connection stays open while the student uses the menu.

## Run across two computers on a local network

1. On the restaurant computer, build the project and start **./bin/restaurant_server 5001**.
2. Find the restaurant computer's local IP address. Allow TCP port 5001 through its firewall for your trusted local network.
3. On the student-side computer, start the Student Server and point it to the restaurant computer's IP: **./bin/student_server 5002 RESTAURANT_LAN_IP 5001**.
4. Student clients connect to the student-side computer: **./bin/student_client STUDENT_SERVER_LAN_IP 5002**.
5. Restaurant staff can run the admin client from a computer that can reach the restaurant server: **./bin/restaurant_admin RESTAURANT_LAN_IP 5001**.

Replace the IP placeholders with the actual local IP addresses. Both computers must be on a network that allows direct TCP connections. Do not expose these ports to the public internet.

## Test the project

Build and run the automated integration test:

~~~bash
sh ./build.sh
python3 test_network.py
~~~

The test uses temporary data files and temporary ports. It checks both servers, student registration/login, menu retrieval, order totals, cancellation, admin PIN validation, menu availability changes, and data persistence after restarting servers. It does not modify your normal project data files.

## Data files

The servers create these text files in their **current working directory**:

- students_data.txt — student account records
- menu_data.txt — menu and availability
- restaurant_data.txt — orders and cancellation status

When started with run_local.sh, they are created in the project folder. These local data files are excluded from Git by .gitignore because they may contain names and order records.

## Important classroom-demo limitations

This is an educational LAN project, not a production ordering or payment system. Messages are plain text over TCP. The default admin PIN (1234) is only for demonstrating the feature and should be changed for a class demo. Student passwords are hashed with the C++ standard library's std::hash; that is not secure password storage for real accounts. Use the project only on a trusted network.
