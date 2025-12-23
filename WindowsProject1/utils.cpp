#include "utils.h"

std::wstring roomTypeToString(RoomType t) {
    switch (t) {
    case RoomType::Single:
        return L"Single";
    case RoomType::Double:
        return L"Double";
    case RoomType::Lux:
        return L"Lux";
    default:
        return L"Unknown";
    }
}
