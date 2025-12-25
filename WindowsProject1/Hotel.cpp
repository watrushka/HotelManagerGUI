#include "Room.h"
#include "Client.h"
#include "Hotel.h"
#include <algorithm>  // для std::sort
#include <fstream>      // для wifstream / wofstream
#include <string>       // для std::wstring
#include <sstream>      // для std::wstringstream
#include <locale>
#include <codecvt>
#include "database.h"

using namespace std;



bool Hotel::addRoom(const Room& r) {
    for (auto& room : rooms)
        if (room.getId() == r.getId()) return false;
    rooms.push_back(r);
    return true;
}

bool Hotel::registerClient(const wstring& surname, int roomId, int days) {
    for (auto& room : rooms) {
        if (room.getId() == roomId && !room.isOccupied()) {
            room.occupy();
            clients.emplace_back(surname, roomId, days);
            return true;
        }
    }
    return false;
}

vector<Room> Hotel::getFreeRooms() const {
    vector<Room> freeRooms;
    for (auto& room : rooms)
        if (!room.isOccupied()) freeRooms.push_back(room);
    return freeRooms;
}

double Hotel::getClientCost(const std::wstring& surname) const {
    for (const auto& client : clients) {
        if (client.getSurname() == surname) {
            for (const auto& room : rooms) {
                if (room.getId() == client.getRoomId()) {
                    return client.calcCost(room.getPrice());
                }
            }
        }
    }
    return -1;
}

const vector<Client>& Hotel::getClients() const { return clients; }

void Hotel::removeClientByIndex(int idx) {
    if (idx >= 0 && idx < clients.size()) {
        int roomId = clients[idx].getRoomId();

        // Освобождаем комнату
        for (auto& r : rooms) {
            if (r.getId() == roomId) {
                r.vacate(); // нужно добавить метод vacate() в Room
                break;
            }
        }

        // Удаляем клиента
        clients.erase(clients.begin() + idx);
    }
}


void Hotel::updateClient(int idx, const Client& c) {
    if (idx >= 0 && idx < clients.size()) clients[idx] = c;
}

//void Hotel::sortClientsBySurname() {
//    sort(clients.begin(), clients.end(),
//        [](const Client& a, const Client& b) { return a.getSurname() < b.getSurname(); });
//}
//
//void Hotel::sortClientsByCost() {
//    sort(clients.begin(), clients.end(),
//        [this](const Client& a, const Client& b) {
//            return a.calcCost(getRoomPrice(a.getRoomId())) < a.calcCost(getRoomPrice(b.getRoomId()));
//        });
//}

// Сортировка комнат по цене за день (по возрастанию)
void Hotel::sortRoomsByPriceAsc() {
    std::sort(rooms.begin(), rooms.end(),
        [](const Room& a, const Room& b) {
            return a.getPrice() < b.getPrice();
        });
}

// Сортировка комнат по цене за день (по убыванию)
void Hotel::sortRoomsByPriceDesc() {
    std::sort(rooms.begin(), rooms.end(),
        [](const Room& a, const Room& b) {
            return a.getPrice() > b.getPrice();
        });
}


// Сортировка комнат по типу (Single < Double < Lux)
void Hotel::sortRoomsByTypeAsc() {
    std::sort(rooms.begin(), rooms.end(),
        [](const Room& a, const Room& b) {
            return a.getType() < b.getType(); // предполагаем, что RoomType — это enum с порядком
        });
}

void Hotel::sortRoomsByTypeDesc() {
    std::sort(rooms.begin(), rooms.end(),
        [](const Room& a, const Room& b) {
            return a.getType() > b.getType(); // предполагаем, что RoomType — это enum с порядком
        });
}

// Сортировка комнат по ID (по возрастанию)
void Hotel::sortRoomsByIdAsc() {
    std::sort(rooms.begin(), rooms.end(),
        [](const Room& a, const Room& b) {
            return a.getId() < b.getId();
        });
}

// Сортировка комнат по ID (по убыванию)
void Hotel::sortRoomsByIdDesc() {
    std::sort(rooms.begin(), rooms.end(),
        [](const Room& a, const Room& b) {
            return a.getId() > b.getId();
        });
}



double Hotel::getRoomPrice(int roomId) const {
    for (const auto& room : rooms)
        if (room.getId() == roomId) return room.getPrice();
    return 0;
}

bool Hotel::saveToFile(const std::wstring& filename) const {
    std::wofstream fout(filename, std::ios::binary);
    if (!fout) return false;

    // Устанавливаем UTF-8 с BOM
    fout.imbue(std::locale(fout.getloc(),
        new std::codecvt_utf8_utf16<wchar_t, 0x10ffff, std::little_endian>));

    // Сначала комнаты
    fout << L"#ROOMS\n";
    for (const auto& r : rooms) {
        fout << r.getId() << L"\t"
            << r.getPrice() << L"\t"
            << (int)r.getType() << L"\t"
            << r.isOccupied() << L"\n";
    }

    // Потом клиенты
    fout << L"#CLIENTS\n";
    for (const auto& c : clients) {
        fout << c.getSurname() << L"\t"
            << c.getRoomId() << L"\t"
            << c.getDays() << L"\n";
    }

    return true;
}


bool Hotel::loadFromFile(const std::wstring& filename) {
    std::wifstream fin(filename, std::ios::binary);
    if (!fin) return false;

    fin.imbue(std::locale(fin.getloc(),
        new std::codecvt_utf8_utf16<wchar_t, 0x10ffff, std::little_endian>));

    rooms.clear();
    clients.clear();

    std::wstring line;
    enum class Section { NONE, ROOMS, CLIENTS };
    Section sec = Section::NONE;

    while (std::getline(fin, line)) {
        if (line.empty()) continue;
        if (line == L"#ROOMS") { sec = Section::ROOMS; continue; }
        if (line == L"#CLIENTS") { sec = Section::CLIENTS; continue; }

        std::wstringstream ss(line);
        if (sec == Section::ROOMS) {
            int id, typeInt, occupied;
            double price;
            ss >> id >> price >> typeInt >> occupied;
            RoomType type = static_cast<RoomType>(typeInt);
            Room r(id, price, type);
            if (occupied) r.occupy();
            rooms.push_back(r);
        }
        else if (sec == Section::CLIENTS) {
            std::wstring surname;
            int roomId, days;
            ss >> surname >> roomId >> days;
            clients.emplace_back(surname, roomId, days);
        }
    }

    return true;
}

bool Hotel::loadFromDatabase(Database& db) {
    rooms = db.loadRooms();
    clients = db.loadClients();
    return true;
}

void Hotel::saveToDatabase(Database& db) {
    // Полностью очищаем таблицы
    db.exec("DELETE FROM clients;");
    db.exec("DELETE FROM rooms;");

    // Вставляем заново все комнаты и клиентов
    for (const auto& r : rooms)
        db.insertRoom(r);
    for (const auto& c : clients)
        db.insertClient(c);
}





const Room* Hotel::getRoomById(int id) const {
    for (const auto& room : rooms) {
        if (room.getId() == id) {
            return &room;
        }
    }
    return nullptr;
}

Room* Hotel::getRoomById(int id) {
    for (auto& room : rooms) {
        if (room.getId() == id)
            return &room;
    }
    return nullptr;
}

bool Hotel::isRoomExists(int id) const {
    for (const auto& room : rooms) {
        if (room.getId() == id) return true;
    }
    return false;
}

bool Hotel::removeRoomById(int id) {
    // Удаляем всех клиентов, которые занимают эту комнату
    for (int i = static_cast<int>(clients.size()) - 1; i >= 0; i--) {
        if (clients[i].getRoomId() == id)
            clients.erase(clients.begin() + i);
    }

    // Удаляем саму комнату
    auto it = std::find_if(rooms.begin(), rooms.end(), [id](const Room& r) { return r.getId() == id; });
    if (it != rooms.end()) {
        rooms.erase(it);
        return true;
    }

    return false; // комната не найдена
}

bool Hotel::isRoomFree(int roomId) const {
    for (const auto& c : clients)
        if (c.getRoomId() == roomId)
            return false;
    return true;
}




vector<Room>& Hotel::getRooms() { return rooms; }