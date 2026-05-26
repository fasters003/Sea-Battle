#pragma once

enum class CellStatus { Empty, Ship, Miss, Hit };

class Cell {
public:
    CellStatus status;
    bool hasMine;
    bool isIlluminated;

    Cell();
};
