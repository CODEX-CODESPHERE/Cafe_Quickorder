#include "net.hpp"
#include <iostream>
#include <string>
using namespace std;

int main(int argc, char** argv) {
    string host = argc > 1 ? argv[1] : "127.0.0.1";
    int port = 5001;
    try { if (argc > 2) port = stoi(argv[2]); }
    catch (...) { cerr << "Port must be a number.\n"; return 1; }

    cout << "=== RESTAURANT ADMIN TERMINAL ===\n";
    cout << "Demo PIN is 1234 (change ADMIN_PIN in restaurant_server.cpp for your class demo).\n";
    cout << "Admin PIN: ";
    string pin;
    getline(cin, pin);
    pin = cleanField(pin);

    string login = request(host, port, "ADMIN|" + pin + "|CHECK");
    cout << login << '\n';
    if (login.rfind("OK|", 0) != 0) return 1;

    while (true) {
        cout << "\n--- RESTAURANT MENU ---\n"
             << "1 View food menu\n"
             << "2 View all orders\n"
             << "3 Sales report\n"
             << "4 Update food availability\n"
             << "0 Exit\nChoice: ";
        string choice;
        getline(cin, choice);
        string action;
        if (choice == "1") action = "MENU";
        else if (choice == "2") action = "ALLORDERS";
        else if (choice == "3") action = "REPORT";
        else if (choice == "4") {
            string id, available;
            cout << "Item ID: "; getline(cin, id);
            cout << "Available? (1=yes, 0=no): "; getline(cin, available);
            action = "AVAIL|" + cleanField(id) + "|" + cleanField(available);
        } else if (choice == "0") break;
        else { cout << "Invalid choice. Try again.\n"; continue; }

        cout << request(host, port, "ADMIN|" + pin + "|" + action) << '\n';
    }
    cout << "Restaurant admin closed.\n";
    return 0;
}
