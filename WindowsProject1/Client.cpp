#include "Client.h"

Client::Client(wstring s, int roomId, int days) : surname(s), roomId(roomId), days(days) {}
const std::wstring& Client::getSurname() const {
    return surname;
}
int Client::getRoomId() const { return roomId; }
int Client::getDays() const { return days; }
double Client::calcCost(double price) const { return price * days; }
