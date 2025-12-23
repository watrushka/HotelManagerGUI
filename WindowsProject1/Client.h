#pragma once
#include <string>
using namespace std;

class Client {
private:
    wstring surname;
    int roomId;
    int days;
public:
    Client(wstring s, int roomId, int days);
    const std::wstring& getSurname() const;
    int getRoomId() const;
    int getDays() const;
    double calcCost(double price) const;
};
