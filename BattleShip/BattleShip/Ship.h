#pragma once
#include <vector>

struct ShipPart {
    int x, y;
    bool isHit = false;
};

class Ship {
public:
    int size;
    bool isHorizontal;
    std::vector<ShipPart> parts;

    Ship(int s, int startX, int startY, bool hor);
    bool isSunk();
};
