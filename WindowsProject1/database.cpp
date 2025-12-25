#include "database.h"
#include <windows.h>
#include <vector>
#include <string>
#include <stdexcept>
#include "sqlite3.h"


Database::Database() : db(nullptr) {}

Database::~Database() {
    close();
}

bool Database::open(const std::wstring& path) {
    return sqlite3_open16(path.c_str(), (sqlite3**)&db) == SQLITE_OK;
}

void Database::close() {
    if (db) {
        sqlite3_close((sqlite3*)db);
        db = nullptr;
    }
}

bool Database::init() {
    const char* sqlRooms =
        "CREATE TABLE IF NOT EXISTS rooms ("
        "id INTEGER PRIMARY KEY,"
        "type INTEGER,"
        "price REAL,"
        "occupied INTEGER);";

    const char* sqlClients =
        "CREATE TABLE IF NOT EXISTS clients ("
        "surname TEXT,"
        "roomId INTEGER,"
        "days INTEGER);";

    char* err = nullptr;
    if (sqlite3_exec((sqlite3*)db, sqlRooms, nullptr, nullptr, &err) != SQLITE_OK)
        return false;

    if (sqlite3_exec((sqlite3*)db, sqlClients, nullptr, nullptr, &err) != SQLITE_OK)
        return false;

    return true;
}

bool Database::insertRoom(const Room& room) {
    const char* sql = "INSERT INTO rooms (id, type, price, occupied) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2((sqlite3*)db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, room.getId());
    sqlite3_bind_int(stmt, 2, static_cast<int>(room.getType()));
    sqlite3_bind_double(stmt, 3, room.getPrice());
    sqlite3_bind_int(stmt, 4, room.isOccupied() ? 1 : 0);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::deleteRoom(int roomId) {
    const char* sql = "DELETE FROM rooms WHERE id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2((sqlite3*)db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, roomId);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

// Аналогично для клиентов:
bool Database::insertClient(const Client& c) {
    const char* sql = "INSERT INTO clients (surname, roomId, days) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2((sqlite3*)db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text16(stmt, 1, c.getSurname().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, c.getRoomId());
    sqlite3_bind_int(stmt, 3, c.getDays());

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::deleteClient(int roomId) {
    const char* sql = "DELETE FROM clients WHERE roomId = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2((sqlite3*)db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, roomId);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<Client> Database::loadClients() {
    std::vector<Client> clients;
    const char* sql = "SELECT surname, roomId, days FROM clients;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2((sqlite3*)db, sql, -1, &stmt, nullptr);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const wchar_t* surname = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 0));
        int roomId = sqlite3_column_int(stmt, 1);
        int days = sqlite3_column_int(stmt, 2);

        clients.emplace_back(surname, roomId, days);
    }

    sqlite3_finalize(stmt);
    return clients;
}


std::vector<Room> Database::loadRooms() {
    std::vector<Room> rooms;
    const char* sql = "SELECT id, type, price, occupied FROM rooms;";

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2((sqlite3*)db, sql, -1, &stmt, nullptr);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        int type = sqlite3_column_int(stmt, 1);
        double price = sqlite3_column_double(stmt, 2);
        bool occupied = sqlite3_column_int(stmt, 3);

        Room r(id, price, (RoomType)type);
        if (occupied) r.occupy();

        rooms.push_back(r);
    }

    sqlite3_finalize(stmt);
    return rooms;
}

bool Database::exec(const std::string& sql) {
    char* err = nullptr;
    if (sqlite3_exec((sqlite3*)db, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        if (err) {
            sqlite3_free(err);
        }
        return false;
    }
    return true;
}

bool Database::exec(const std::wstring& sql) {
    char* err = nullptr;
    // Преобразуем wstring в UTF-8
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, sql.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8sql(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, sql.c_str(), -1, &utf8sql[0], size_needed, nullptr, nullptr);

    return exec(utf8sql);
}
