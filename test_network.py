#!/usr/bin/env python3
"""Small end-to-end test for the beginner-level two-server C++ project."""
import os
import socket
import subprocess
import tempfile
import time
import uuid
import shutil

ROOT = os.path.dirname(os.path.abspath(__file__))
BIN = os.path.join(ROOT, "bin")


def free_port():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def request(sock, message):
    sock.settimeout(3)
    sock.sendall((message + "\n").encode())
    data = bytearray()
    while not data.endswith(b"\n"):
        chunk = sock.recv(4096)
        if not chunk:
            raise RuntimeError("Server closed the connection unexpectedly")
        data.extend(chunk)
    return data.decode().strip()


def one_shot(port, message):
    with socket.create_connection(("127.0.0.1", port), timeout=3) as sock:
        return request(sock, message)


def wait_for_server(port, process):
    for _ in range(40):
        if process.poll() is not None:
            raise RuntimeError(f"Server stopped early with exit code {process.returncode}")
        try:
            if one_shot(port, "PING").startswith("OK|"):
                return
        except (OSError, RuntimeError):
            pass
        time.sleep(0.1)
    raise RuntimeError(f"Server on port {port} did not start")


def stop(process):
    if process and process.poll() is None:
        process.terminate()
        process.wait(timeout=3)


def start_restaurant(port, temp_dir):
    process = subprocess.Popen(
        [os.path.join(BIN, "restaurant_server"), str(port)], cwd=temp_dir,
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
    )
    wait_for_server(port, process)
    return process


def start_student(port, restaurant_port, temp_dir):
    process = subprocess.Popen(
        [os.path.join(BIN, "student_server"), str(port), "127.0.0.1", str(restaurant_port)],
        cwd=temp_dir, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
    )
    wait_for_server(port, process)
    return process


def main():
    if not os.path.exists(os.path.join(BIN, "restaurant_server")):
        subprocess.run(["./build.sh"], cwd=ROOT, check=True)

    tag = uuid.uuid4().hex[:7]
    student_id = "TEST" + tag
    rport = free_port()
    sport = free_port()
    while sport == rport:
        sport = free_port()

    temp_dir = tempfile.mkdtemp(prefix="cafe_quickorder_test_")
    restaurant = student = None
    try:
        restaurant = start_restaurant(rport, temp_dir)
        student = start_student(sport, rport, temp_dir)

        registered = one_shot(sport, f"REGISTER|{student_id}|Integration Test|pw123")
        assert registered.startswith("OK|"), registered

        with socket.create_connection(("127.0.0.1", sport), timeout=3) as sock:
            assert request(sock, f"LOGIN|{student_id}|pw123").startswith("OK|")
            assert "Sandwich" in request(sock, "MENU")
            placed = request(sock, "PLACE|Test Pickup|1:2,3:1")
            assert "Total: Rs.190" in placed, placed
            order_id = placed.split("Order ID: ")[1].split()[0]
            assert "ACTIVE" in request(sock, "MYORDERS")
            assert request(sock, "ALLORDERS").startswith("ERR|")
            assert request(sock, "CANCEL|" + order_id).startswith("OK|")
            assert "CANCELLED" in request(sock, "MYORDERS")

        assert one_shot(rport, "ADMIN|wrong|REPORT").startswith("ERR|")
        assert one_shot(rport, "REPORT").startswith("ERR|"), "Admin report must require PIN"
        assert one_shot(rport, "ADMIN|1234|CHECK").startswith("OK|")
        assert "CANCELLED" in one_shot(rport, "ADMIN|1234|ALLORDERS")
        assert "Revenue: Rs.0" in one_shot(rport, "ADMIN|1234|REPORT")
        assert "Availability updated" in one_shot(rport, "ADMIN|1234|AVAIL|2|0")
        assert "Veg Burger" in one_shot(rport, "MENU")
        assert one_shot(rport, f"PLACE|{student_id}|Test Pickup|2:1").startswith("ERR|")

        # The menu and cancelled order should remain after restarting the restaurant server.
        stop(restaurant)
        restaurant = start_restaurant(rport, temp_dir)
        assert "CANCELLED" in one_shot(rport, "ADMIN|1234|ALLORDERS")
        menu_after_restart = one_shot(rport, "MENU")
        assert "Veg Burger" in menu_after_restart and "Unavailable" in menu_after_restart

        # Student records should remain after restarting the student server too.
        stop(student)
        student = start_student(sport, rport, temp_dir)
        with socket.create_connection(("127.0.0.1", sport), timeout=3) as sock:
            assert request(sock, f"LOGIN|{student_id}|pw123").startswith("OK|")
            assert "Sandwich" in request(sock, "MENU")

        print("PASS: both servers start and communicate over TCP")
        print("PASS: student registration/login, menu, ordering, billing and cancellation")
        print("PASS: students cannot access restaurant admin reports")
        print("PASS: admin PIN, order report and menu availability update")
        print("PASS: student, order and menu data survive server restarts")
    finally:
        stop(student)
        stop(restaurant)
        shutil.rmtree(temp_dir, ignore_errors=True)


if __name__ == "__main__":
    main()
