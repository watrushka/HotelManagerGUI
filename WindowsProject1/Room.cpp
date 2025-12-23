#include "Room.h"
#include "string"

Room::Room(int id, double price, RoomType type) : id(id), price(price), type(type), occupied(false) {}
int Room::getId() const { return id; }
double Room::getPrice() const { return price; }
RoomType Room::getType() const { return type; }
bool Room::isOccupied() const { return occupied; }
bool Room::occupy() {
    if (occupied) return false;
    occupied = true;
    return true;
}
void Room::vacate() { occupied = false; }


