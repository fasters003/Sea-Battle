#pragma once
#include <vector>
#include <SFML/Graphics.hpp>
#include "Cell.h"
#include "Ship.h"

enum class BotDifficulty { Easy, Hard, Ultra };

class Field {
private:
    std::vector<std::vector<Cell>> cells;
    std::vector<Ship> ships;
    sf::RectangleShape shape;

    bool trackingShip = false;
    int lastHitX = -1;
    int lastHitY = -1;

    std::vector<std::vector<Cell>> backupCells;
    std::vector<Ship> backupShips;

public:
    int abilityPoints = 0;

    Field();
    void clearField();
    void draw(sf::RenderWindow& window, float offsetX, float offsetY, bool isEnemy);

    int handleInputForBot(int j, int i);
    int handleSmartBotInput(BotDifficulty diff);

    bool canPlaceShip(int x, int y, int size, bool horizontal);
    void placeShipManually(int x, int y, int size, bool horizontal);
    void placeShipsRandomly();
    void undoLastShip(std::vector<int>& remainingShips);

    void useRadar(sf::Vector2i mousePos, float offsetX, float offsetY);
    void useSpotlight(int i, int j);

    bool allShipsDestroyed();
    size_t getShipsCount() const { return ships.size(); }

    void saveState();
    void restoreState();
};