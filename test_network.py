#!/usr/bin/env python3
"""End-to-end smoke test for the two-server Cafe QuickOrder system."""
import os, socket, subprocess, tempfile, time, uuid, shutil

ROOT = os.path.dirname(os.path.abspath(__file__))
BIN = os.path.join(ROOT, "bin")
if not os.path.exists(os.path.join(BIN, "restaurant_server")):
    subprocess.run(["./build.sh"], cwd=ROOT, check=True)
tag = uuid.uuid4().hex[:7]
student_id = "TEST" + tag
rport, sport = 56131, 56132
tmp = tempfile.mkdtemp(prefix="cafe_quickorder_test_")
restaurant = student = None

def request(sock, message):
    sock.sendall((message + "\n").encode())
    data = bytearray()
    while not data.endswith(b"\n"):
        chunk = sock.recv(4096)
        if not chunk:
            raise RuntimeError("Server closed connection unexpectedly")
        data.extend(chunk)
    return data.decode().strip()

def one_shot(port, message):
    with socket.create_connection(("127.0.0.1", port), timeout=3) as s:
        return request(s, message)

try:
    restaurant = subprocess.Popen([os.path.join(BIN, "restaurant_server"), str(rport)],
                                  cwd=tmp, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(.3)
    student = subprocess.Popen([os.path.join(BIN, "student_server"), str(sport),
                                "127.0.0.1", str(rport)], cwd=tmp,
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(.4)
    assert one_shot(sport, f"REGISTER|{student_id}|Integration Test|pw123").startswith("OK|")
    with socket.create_connection(("127.0.0.1", sport), timeout=3) as s:
        assert request(s, f"LOGIN|{student_id}|pw123").startswith("OK|")
        assert "Sandwich" in request(s, "MENU")
        placed = request(s, "PLACE|Test Pickup|1:2,3:1")
        assert "Total: Rs.190" in placed, placed
        order_id = placed.split("Order ID: ")[1].split()[0]
        assert "ACTIVE" in request(s, "MYORDERS")
        assert request(s, "ALLORDERS").startswith("ERR|")
        assert request(s, "CANCEL|" + order_id).startswith("OK|")
        assert "CANCELLED" in request(s, "MYORDERS")
    assert "Revenue: Rs.0" in one_shot(rport, "REPORT")
    # Verify restaurant data survives a server restart.
    restaurant.terminate(); restaurant.wait(timeout=3); restaurant = None
    restaurant = subprocess.Popen([os.path.join(BIN, "restaurant_server"), str(rport)],
                                  cwd=tmp, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(.3)
    assert "CANCELLED" in one_shot(rport, "ALLORDERS")
    assert "TEST" + tag in open(os.path.join(tmp, "students_data.txt")).read()
    print("PASS: registration, login, menu, network ordering, total calculation,")
    print("      student order privacy, cancellation, sales reporting and persistence.")
finally:
    if student:
        student.terminate()
        student.wait(timeout=3)
    if restaurant:
        restaurant.terminate()
        restaurant.wait(timeout=3)
    shutil.rmtree(tmp, ignore_errors=True)
