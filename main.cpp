#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>
#include "httplib.h"
#include <nlohmann/json.hpp>
#include <date/date.h>

using namespace std;
using namespace std::chrono;
using json = nlohmann::json;
using namespace date;

// --------------------------
// Базовые модели данных
// --------------------------

struct Guest {
    int id;
    string name;
    string email;
    string phone;
    int loyalty_points = 0;
    vector<int> bookings;
};

struct Room {
    int number;
    string type;
    double price;
    int capacity;
    string status = "available"; // available, occupied, cleaning
    vector<system_clock::time_point> cleaning_schedule;
};

struct Booking {
    int id;
    int guest_id;
    vector<int> room_numbers;
    system_clock::time_point check_in;
    system_clock::time_point check_out;
    string status = "reserved"; // reserved, checked-in, checked-out
    vector<json> services;
};

struct Invoice {
    int id;
    int booking_id;
    vector<json> items;
    bool paid = false;
};

// --------------------------
// Менеджер отеля (основная логика)
// --------------------------

class HotelManager {
private:
    mutex mtx;
    unordered_map<int, Guest> guests;
    unordered_map<int, Room> rooms;
    unordered_map<int, Booking> bookings;
    unordered_map<int, Invoice> invoices;

    struct IdGenerator {
        int guest = 1;
        int room = 1;
        int booking = 1;
        int invoice = 1;
    } ids;

public:
    // Управление гостями
    int add_guest(const string& name, const string& email, const string& phone) {
        lock_guard<mutex> lock(mtx);
        Guest guest{ids.guest++, name, email, phone};
        guests[guest.id] = guest;
        return guest.id;
    }

    // Управление номерами
    int add_room(const string& type, double price, int capacity) {
        lock_guard<mutex> lock(mtx);
        Room room{ids.room++, type, price, capacity};
        rooms[room.number] = room;
        return room.number;
    }

    vector<int> get_available_rooms(const system_clock::time_point& check_in,
                                   const system_clock::time_point& check_out,
                                   const string& room_type = "") {
        lock_guard<mutex> lock(mtx);
        vector<int> available;

        for (const auto& [number, room] : rooms) {
            if (!room_type.empty() && room.type != room_type) continue;
            if (is_room_available(number, check_in, check_out)) {
                available.push_back(number);
            }
        }
        return available;
    }

    bool is_room_available(int room_number,
                          const system_clock::time_point& check_in,
                          const system_clock::time_point& check_out) {
        auto& room = rooms.at(room_number);
        if (room.status != "available") return false;

        for (const auto& [id, booking] : bookings) {
            if (booking.status == "checked-out") continue;
            if (find(booking.room_numbers.begin(),
                    booking.room_numbers.end(), room_number) != booking.room_numbers.end()) {
                if (check_in < booking.check_out && check_out > booking.check_in) {
                    return false;
                }
            }
        }
        return true;
    }

    // Бронирования
    int create_booking(int guest_id, const vector<int>& room_numbers,
                     const system_clock::time_point& check_in,
                     const system_clock::time_point& check_out) {
        lock_guard<mutex> lock(mtx);
        Booking booking{ids.booking++, guest_id, room_numbers, check_in, check_out};
        bookings[booking.id] = booking;
        guests[guest_id].bookings.push_back(booking.id);
        return booking.id;
    }

    bool check_in(int booking_id) {
        lock_guard<mutex> lock(mtx);
        auto it = bookings.find(booking_id);
        if (it == bookings.end() || it->second.status != "reserved") return false;

        it->second.status = "checked-in";
        for (int room_number : it->second.room_numbers) {
            rooms[room_number].status = "occupied";
        }
        return true;
    }

    optional<Invoice> check_out(int booking_id) {
        lock_guard<mutex> lock(mtx);
        auto it = bookings.find(booking_id);
        if (it == bookings.end() || it->second.status != "checked-in") return nullopt;

        it->second.status = "checked-out";

        // Создание счета
        Invoice invoice{ids.invoice++, booking_id};
        auto days = duration_cast<hours>(it->second.check_out - it->second.check_in).count() / 24.0;

        for (int room_number : it->second.room_numbers) {
            auto& room = rooms[room_number];
            invoice.items.push_back({
                {"description", "Room " + to_string(room.number) + " (" + room.type + ")"},
                {"amount", room.price * days}
            });
        }

        for (auto& service : it->second.services) {
            invoice.items.push_back(service);
        }

        invoices[invoice.id] = invoice;

        // Пометить номера для уборки
        for (int room_number : it->second.room_numbers) {
            rooms[room_number].status = "cleaning";
        }

        return invoice;
    }
};

// --------------------------
// Вспомогательные функции
// --------------------------

system_clock::time_point parse_iso_time(const string& time_str) {
    istringstream in(time_str);
    date::sys_time<milliseconds> tp; // Добавить явное указание пространства имён
    in >> date::parse("%FT%TZ", tp);
    return tp;
}

string format_iso_time(const system_clock::time_point& tp) {
    return date::format("%FT%TZ", tp); // Добавить явное указание пространства имён
}

// --------------------------
// HTTP сервер и обработчики
// --------------------------

int main() {
    HotelManager manager;
    httplib::Server svr;

    // Инициализация тестовых данных
    manager.add_room("standard", 100.0, 2);
    manager.add_room("deluxe", 200.0, 4);

    // API Endpoints
    svr.Post("/guests", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto data = json::parse(req.body);
            int id = manager.add_guest(
                data["name"],
                data["email"],
                data["phone"]
            );
            res.set_content(json{{"guest_id", id}}.dump(), "application/json");
            res.status = 201;
        } catch (const exception& e) {
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
            res.status = 400;
        }
    });

    svr.Get("/rooms/available", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto check_in = parse_iso_time(req.get_param_value("check_in"));
            auto check_out = parse_iso_time(req.get_param_value("check_out"));
            auto room_type = req.get_param_value("room_type");

            auto available = manager.get_available_rooms(check_in, check_out, room_type);
            res.set_content(json{{"available_rooms", available}}.dump(), "application/json");
        } catch (const exception& e) {
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
            res.status = 400;
        }
    });

    svr.Post("/bookings", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto data = json::parse(req.body);

            // Проверка доступности номеров
            auto check_in = parse_iso_time(data["check_in"]);
            auto check_out = parse_iso_time(data["check_out"]);

            for (int room_number : data["room_numbers"]) {
                if (!manager.is_room_available(room_number, check_in, check_out)) {
                    throw runtime_error("Room " + to_string(room_number) + " not available");
                }
            }

            int booking_id = manager.create_booking(
                data["guest_id"],
                data["room_numbers"].get<vector<int>>(),
                check_in,
                check_out
            );

            res.set_content(json{{"booking_id", booking_id}}.dump(), "application/json");
            res.status = 201;
        } catch (const exception& e) {
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
            res.status = 400;
        }
    });

    svr.Post(R"(/bookings/(\d+)/checkin)", [&](const httplib::Request& req, httplib::Response& res) {
        int booking_id = stoi(req.matches[1]);
        if (manager.check_in(booking_id)) {
            res.set_content(json{{"status", "checked-in"}}.dump(), "application/json");
        } else {
            res.set_content(json{{"error", "Booking not found or invalid status"}}.dump(), "application/json");
            res.status = 400;
        }
    });

    svr.Post(R"(/bookings/(\d+)/checkout)", [&](const httplib::Request& req, httplib::Response& res) {
        int booking_id = stoi(req.matches[1]);
        auto invoice = manager.check_out(booking_id);

        if (invoice) {
            double total = 0;
            for (auto& item : invoice->items) {
                total += item["amount"].get<double>();
            }

            res.set_content(json{
                {"invoice_id", invoice->id},
                {"total", total},
                {"paid", invoice->paid}
            }.dump(), "application/json");
        } else {
            res.set_content(json{{"error", "Checkout failed"}}.dump(), "application/json");
            res.status = 400;
        }
    });

    // Загрузка конфигурации
    int port = 8080;
    string host = "0.0.0.0";

    cout << "Starting server at " << host << ":" << port << endl;
    svr.listen(host.c_str(), port);

    return 0;
}