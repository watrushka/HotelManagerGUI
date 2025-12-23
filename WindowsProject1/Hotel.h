#pragma once
#include <vector>
#include <string>
#include "Room.h"
#include "Client.h"
using namespace std;

class Hotel {
private:
    vector<Room> rooms;
    vector<Client> clients;
public:
    Hotel() = default;
    bool addRoom(const Room& r);
    bool registerClient(const wstring& surname, int roomId, int days);
    vector<Room> getFreeRooms() const;
    double getClientCost(const wstring& surname) const;
    vector<Room>& getRooms();

    // Новый функционал
    const vector<Client>& getClients() const;
    void removeClientByIndex(int idx);
    void updateClient(int idx, const Client& c);
    void sortClientsBySurname();
    void sortClientsByCost();
    bool saveClientsToFile(const wstring& filename) const;
    bool loadClientsFromFile(const wstring& filename);
    double getRoomPrice(int roomId) const;
    const Room* getRoomById(int id) const;
	Room* getRoomById(int id);
	bool isRoomExists(int id) const;
	bool saveToFile(const wstring& filename) const;
	bool loadFromFile(const wstring& filename);
	bool removeRoomById(int id);
	bool isRoomFree(int id) const;
    void sortRoomsByPriceAsc();
    void sortRoomsByPriceDesc();
    void sortRoomsByTypeAsc();
    void sortRoomsByTypeDesc();
    void sortRoomsByIdAsc();
    void sortRoomsByIdDesc();
};
