#include "net.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <mutex>
#include <thread>
#include <iomanip>
#include <algorithm>
using namespace std;

struct Item { int id; string name; int price; bool available; };
struct Line { int item; int qty; };
struct Order { int id; string student; string customer; vector<Line> lines; int total = 0; bool active = true; };

static vector<Item> menu = {
    {1, "Sandwich", 60, true},
    {2, "Veg Burger", 80, true},
    {3, "Cold Coffee", 70, true},
    {4, "Maggi", 50, true},
    {5, "French Fries", 65, true},
    {6, "Masala Dosa", 90, true}
};
static vector<Order> orders;
static int nextId = 1001;
static mutex dataMutex;
static const string ORDERS_DB = "restaurant_data.txt";
static const string MENU_DB = "menu_data.txt";
static const string ADMIN_PIN = "1234"; // Classroom-demo PIN only.

void saveMenu() {
    ofstream file(MENU_DB);
    for (const auto& item : menu) {
        file << item.id << '|' << cleanField(item.name) << '|' << item.price << '|'
             << (item.available ? 1 : 0) << '\n';
    }
}

void loadMenu() {
    ifstream file(MENU_DB);
    string row;
    vector<Item> loaded;
    while (getline(file, row)) {
        vector<string> fields = split(row, '|');
        if (fields.size() != 4) continue;
        try {
            Item item;
            item.id = stoi(fields[0]);
            item.name = fields[1];
            item.price = stoi(fields[2]);
            item.available = fields[3] == "1";
            if (item.id > 0 && !item.name.empty() && item.price >= 0) loaded.push_back(item);
        } catch (...) {
            // Ignore malformed menu rows instead of stopping the server.
        }
    }
    if (!loaded.empty()) menu = loaded;
    else saveMenu(); // Create the default menu file on the first run.
}

void loadOrders() {
    ifstream file(ORDERS_DB);
    string row;
    while (getline(file, row)) {
        vector<string> fields = split(row, '|');
        if (fields.size() < 6) continue;
        try {
            Order order;
            order.id = stoi(fields[0]);
            order.student = fields[1];
            order.customer = fields[2];
            order.total = stoi(fields[3]);
            order.active = fields[4] == "1";
            for (const string& entry : split(fields[5], ';')) {
                vector<string> pair = split(entry, ':');
                if (pair.size() == 2) order.lines.push_back({stoi(pair[0]), stoi(pair[1])});
            }
            orders.push_back(order);
            nextId = max(nextId, order.id + 1);
        } catch (...) {
            // Ignore malformed order rows.
        }
    }
}

void saveOrders() {
    ofstream file(ORDERS_DB);
    for (const auto& order : orders) {
        file << order.id << '|' << cleanField(order.student) << '|'
             << cleanField(order.customer) << '|' << order.total << '|'
             << (order.active ? 1 : 0) << '|';
        for (size_t i = 0; i < order.lines.size(); ++i) {
            if (i) file << ';';
            file << order.lines[i].item << ':' << order.lines[i].qty;
        }
        file << '\n';
    }
}

string menuText() {
    ostringstream out;
    out << "OK|ID  ITEM                 PRICE STATUS | ";
    for (const auto& item : menu) {
        out << item.id << "   " << left << setw(21) << item.name
            << " Rs." << setw(5) << item.price
            << (item.available ? "Available" : "Unavailable") << " ; ";
    }
    return out.str();
}

string allOrdersText() {
    ostringstream out;
    out << "OK|ALL ORDERS | ";
    for (const auto& order : orders) {
        out << order.id << " | Student " << order.student << " | " << order.customer
            << " | Rs." << order.total << " | "
            << (order.active ? "ACTIVE" : "CANCELLED") << " ; ";
    }
    if (orders.empty()) out << "No orders yet";
    return out.str();
}

string reportText() {
    int active = 0, cancelled = 0, revenue = 0;
    for (const auto& order : orders) {
        if (order.active) { ++active; revenue += order.total; }
        else ++cancelled;
    }
    ostringstream out;
    out << "OK|SALES REPORT | Orders: " << orders.size()
        << " | Active: " << active << " | Cancelled: " << cancelled
        << " | Revenue: Rs." << revenue;
    return out.str();
}

string handle(const string& command) {
    vector<string> p = split(command, '|');
    if (p.empty()) return "ERR|Empty command";
    lock_guard<mutex> lock(dataMutex);

    if (p[0] == "PING") return "OK|PONG";
    if (p[0] == "MENU") return menuText(); // Students may view the menu.

    // Admin operations are checked on the server, not only in the admin client.
    if (p[0] == "ADMIN") {
        if (p.size() < 3) return "ERR|Invalid admin command";
        if (p[1] != ADMIN_PIN) return "ERR|Incorrect admin PIN";
        const string& action = p[2];
        if (action == "CHECK" && p.size() == 3) return "OK|Admin login successful";
        if (action == "MENU" && p.size() == 3) return menuText();
        if (action == "ALLORDERS" && p.size() == 3) return allOrdersText();
        if (action == "REPORT" && p.size() == 3) return reportText();
        if (action == "AVAIL" && p.size() == 5) {
            int id;
            try { id = stoi(p[3]); } catch (...) { return "ERR|Invalid item ID"; }
            if (p[4] != "0" && p[4] != "1") return "ERR|Availability must be 1 or 0";
            for (auto& item : menu) {
                if (item.id == id) {
                    item.available = p[4] == "1";
                    saveMenu();
                    return "OK|Availability updated and saved";
                }
            }
            return "ERR|Item not found";
        }
        if (action == "ADD" && p.size() == 5) {
            string name = cleanField(p[3]);
            int price;
            try { price = stoi(p[4]); } catch (...) { return "ERR|Price must be a whole number"; }
            if (name.empty() || price <= 0) return "ERR|Enter a food name and a price greater than zero";
            int id = 1;
            for (const auto& item : menu) id = max(id, item.id + 1);
            menu.push_back({id, name, price, true});
            saveMenu();
            return "OK|Food item added with ID " + to_string(id);
        }
        if (action == "EDIT" && p.size() == 6) {
            int id, price;
            try { id = stoi(p[3]); price = stoi(p[5]); }
            catch (...) { return "ERR|Item ID and price must be whole numbers"; }
            string name = cleanField(p[4]);
            if (name.empty() || price <= 0) return "ERR|Enter a food name and a price greater than zero";
            for (auto& item : menu) {
                if (item.id == id) {
                    item.name = name;
                    item.price = price;
                    saveMenu();
                    return "OK|Food item updated and saved";
                }
            }
            return "ERR|Item not found";
        }
        if (action == "REMOVE" && p.size() == 4) {
            int id;
            try { id = stoi(p[3]); } catch (...) { return "ERR|Invalid item ID"; }
            auto item = find_if(menu.begin(), menu.end(), [id](const Item& x) { return x.id == id; });
            if (item == menu.end()) return "ERR|Item not found";
            menu.erase(item);
            saveMenu();
            return "OK|Food item removed and saved";
        }
        return "ERR|Unknown admin command";
    }

    if (p[0] == "ALLORDERS" || p[0] == "REPORT" || p[0] == "AVAIL")
        return "ERR|Admin PIN required";

    if (p[0] == "PLACE" && p.size() == 4) {
        Order order;
        order.id = nextId;
        order.student = cleanField(p[1]);
        order.customer = cleanField(p[2]);

        for (const string& entry : split(p[3], ',')) {
            vector<string> pair = split(entry, ':');
            if (pair.size() != 2) return "ERR|Invalid item list";
            int id, quantity;
            try { id = stoi(pair[0]); quantity = stoi(pair[1]); }
            catch (...) { return "ERR|Invalid item or quantity"; }
            if (quantity < 1 || quantity > 30) return "ERR|Quantity must be between 1 and 30";
            auto item = find_if(menu.begin(), menu.end(), [id](const Item& x) { return x.id == id; });
            if (item == menu.end() || !item->available) return "ERR|Invalid or unavailable item";

            auto existing = find_if(order.lines.begin(), order.lines.end(), [id](const Line& x) { return x.item == id; });
            if (existing == order.lines.end()) order.lines.push_back({id, quantity});
            else {
                if (existing->qty + quantity > 30) return "ERR|Total quantity per item cannot exceed 30";
                existing->qty += quantity;
            }
            order.total += item->price * quantity;
        }
        if (order.lines.empty()) return "ERR|Order cannot be empty";
        ++nextId;
        orders.push_back(order);
        saveOrders();
        return "OK|Order placed. Order ID: " + to_string(order.id) + " Total: Rs." + to_string(order.total);
    }

    if (p[0] == "MYORDERS" && p.size() == 2) {
        ostringstream out;
        out << "OK|Your orders: ";
        bool found = false;
        for (const auto& order : orders) {
            if (order.student == p[1]) {
                found = true;
                out << "ID " << order.id << " | " << order.customer << " | Rs." << order.total
                    << " | " << (order.active ? "ACTIVE" : "CANCELLED") << " ; ";
            }
        }
        if (!found) out << "No orders found";
        return out.str();
    }

    if (p[0] == "CANCEL" && p.size() == 3) {
        int id;
        try { id = stoi(p[2]); } catch (...) { return "ERR|Invalid order ID"; }
        for (auto& order : orders) {
            if (order.id == id && order.student == p[1]) {
                if (!order.active) return "ERR|Order already cancelled";
                order.active = false;
                saveOrders();
                return "OK|Order cancelled";
            }
        }
        return "ERR|Order not found";
    }

    return "ERR|Unknown command or invalid arguments";
}

void client(int fd) {
    string command;
    while (recvLine(fd, command)) {
        if (!sendLine(fd, handle(command))) break;
    }
    close(fd);
}

int main(int argc, char** argv) {
    int port = 5001;
    try { if (argc > 1) port = stoi(argv[1]); }
    catch (...) { cerr << "Port must be a number.\n"; return 1; }
    if (port < 1 || port > 65535) { cerr << "Port must be from 1 to 65535.\n"; return 1; }
    loadMenu();
    loadOrders();
    int server = listenOn(port);
    if (server < 0) {
        cerr << "Cannot listen on port " << port << ". Is it already in use?\n";
        return 1;
    }
    cout << "Restaurant server listening on port " << port << "\n"
         << "Menu file: " << MENU_DB << "\nOrders file: " << ORDERS_DB
         << "\nPress Ctrl+C to stop.\n";
    while (true) {
        int fd = accept(server, nullptr, nullptr);
        if (fd < 0) continue;
        thread(client, fd).detach();
    }
}
