#include "Ship.h"

Ship::Ship(int s, int startX, int startY, bool hor) : size(s), isHorizontal(hor) {
    for (int i = 0; i < size; ++i) {
        if (isHorizontal) parts.push_back({ startX + i, startY, false });
        else parts.push_back({ startX, startY + i, false });
    }
}

bool Ship::isSunk() {
    for (auto& part : parts) {
        if (!part.isHit) return false;
    }
    return true;
}
