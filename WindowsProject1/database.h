#pragma once
#include <string>
#include <vector>
#include "hotel.h"

class Database {
public:
    Database();
    ~Database();

    bool open(const std::wstring& path);
    void close();

    bool init(); // создание таблиц

    // -------- Rooms --------
    bool insertRoom(const Room& room);
    bool deleteRoom(int roomId);
    std::vector<Room> loadRooms();

    // -------- Clients --------
    bool insertClient(const Client& client);
    bool deleteClient(int clientId);
    std::vector<Client> loadClients();

    // -------- File operations (лаба) --------
    bool exportToFile(const std::wstring& filePath);
    bool importFromFile(const std::wstring& filePath);

    // Выполнение произвольного SQL-запроса
    bool exec(const std::string& sql);
    bool exec(const std::wstring& sql); // для удобства с wide string



private:
    void* db; // sqlite3*
};
