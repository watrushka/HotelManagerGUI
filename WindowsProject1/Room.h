// Room.h
#pragma once

enum class RoomType {
    Single = 0,
    Double,
    Lux
};

class Room {
private:
    int id;
    double price;
    RoomType type;
    bool occupied;
public:
    Room(int id, double price, RoomType type);

    int getId() const;
    double getPrice() const;
    RoomType getType() const;

    bool isOccupied() const;
    bool occupy();
    void vacate();
};
