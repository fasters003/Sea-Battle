#include "Field.h"
#include <ctime>
#include <cstdlib>

Field::Field() {
    clearField();
    shape.setSize(sf::Vector2f(38.f, 38.f));
    shape.setOutlineThickness(1.f);
}

void Field::clearField() {
    cells.assign(10, std::vector<Cell>(10));
    ships.clear();
    abilityPoints = 0;
    trackingShip = false;
    lastHitX = lastHitY = -1;
}

void Field::draw(sf::RenderWindow& window, float offsetX, float offsetY, bool isEnemy) {
    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < 10; ++j) {
            shape.setPosition(offsetX + j * 40.f, offsetY + i * 40.f);
            CellStatus s = cells[i][j].status;

            if (s == CellStatus::Empty) shape.setFillColor(sf::Color(0, 100, 200));
            else if (s == CellStatus::Ship) shape.setFillColor(isEnemy ? sf::Color(0, 100, 200) : sf::Color(120, 120, 120));
            else if (s == CellStatus::Miss) shape.setFillColor(sf::Color(0, 50, 100));
            else if (s == CellStatus::Hit) shape.setFillColor(sf::Color::Red);

            if (cells[i][j].isIlluminated && (s == CellStatus::Empty || s == CellStatus::Ship)) {
                shape.setFillColor(s == CellStatus::Ship ? sf::Color::Yellow : sf::Color(100, 200, 255));
            }

            shape.setOutlineColor(sf::Color::White);
            shape.setOutlineThickness(1.f);
            window.draw(shape);
        }
    }

    for (auto& ship : ships) {
        if (ship.isSunk()) {
            for (auto& part : ship.parts) {
                float cx = offsetX + part.x * 40.f + 19.f;
                float cy = offsetY + part.y * 40.f + 19.f;

                sf::RectangleShape line1(sf::Vector2f(44.f, 4.f));
                line1.setOrigin(22.f, 2.f);
                line1.setPosition(cx, cy);
                line1.setFillColor(sf::Color::Black);
                line1.setRotation(45.f);

                sf::RectangleShape line2(sf::Vector2f(44.f, 4.f));
                line2.setOrigin(22.f, 2.f);
                line2.setPosition(cx, cy);
                line2.setFillColor(sf::Color::Black);
                line2.setRotation(-45.f);

                window.draw(line1);
                window.draw(line2);
            }
        }
    }
}

int Field::handleInputForBot(int j, int i) {
    if (i < 0 || i >= 10 || j < 0 || j >= 10) return 0;
    if (cells[i][j].status == CellStatus::Miss || cells[i][j].status == CellStatus::Hit) return 0;

    cells[i][j].isIlluminated = false;

    if (cells[i][j].status == CellStatus::Empty) {
        cells[i][j].status = CellStatus::Miss;
        return -1;
    }
    else if (cells[i][j].status == CellStatus::Ship) {
        cells[i][j].status = CellStatus::Hit;

        for (auto& ship : ships) {
            for (auto& part : ship.parts) {
                if (part.x == j && part.y == i) {
                    part.isHit = true;
                    if (ship.isSunk()) {
                        for (auto& sp : ship.parts) {
                            for (int dy = -1; dy <= 1; dy++)
                                for (int dx = -1; dx <= 1; dx++) {
                                    int nx = sp.x + dx, ny = sp.y + dy;
                                    if (nx >= 0 && nx < 10 && ny >= 0 && ny < 10 &&
                                        cells[ny][nx].status == CellStatus::Empty)
                                        cells[ny][nx].status = CellStatus::Miss;
                                }
                        }
                        trackingShip = false;
                        lastHitX = lastHitY = -1;
                        return ship.size;
                    }
                    lastHitX = j;
                    lastHitY = i;
                    trackingShip = true;
                    return 0;
                }
            }
        }
    }
    return 0;
}

int Field::handleSmartBotInput(BotDifficulty diff) {
    if (diff == BotDifficulty::Easy) {
        int x = rand() % 10, y = rand() % 10;
        int attempts = 0;
        while ((cells[y][x].status == CellStatus::Miss || cells[y][x].status == CellStatus::Hit) && attempts < 300) {
            x = rand() % 10; y = rand() % 10;
            attempts++;
        }
        return handleInputForBot(x, y);
    }

    if (diff == BotDifficulty::Hard || diff == BotDifficulty::Ultra) {
        std::vector<std::pair<int, int>> huntTargets;

        for (int i = 0; i < 10; ++i) {
            for (int j = 0; j < 10; ++j) {
                if (cells[i][j].status == CellStatus::Hit) {

                    bool horiz = (j > 0 && cells[i][j - 1].status == CellStatus::Hit) || (j < 9 && cells[i][j + 1].status == CellStatus::Hit);
                    bool vert = (i > 0 && cells[i - 1][j].status == CellStatus::Hit) || (i < 9 && cells[i + 1][j].status == CellStatus::Hit);

                    int dx[] = { 0, 0, 1, -1 };
                    int dy[] = { 1, -1, 0, 0 };

                    for (int d = 0; d < 4; ++d) {
                        int nx = j + dx[d], ny = i + dy[d];

                        if (nx >= 0 && nx < 10 && ny >= 0 && ny < 10 &&
                            cells[ny][nx].status != CellStatus::Miss &&
                            cells[ny][nx].status != CellStatus::Hit) {

                            if (horiz && dx[d] == 0) continue;
                            if (vert && dy[d] == 0) continue;

                            huntTargets.push_back({ nx, ny });
                        }
                    }
                }
            }
        }

        if (!huntTargets.empty()) {
            int r = rand() % huntTargets.size();
            return handleInputForBot(huntTargets[r].first, huntTargets[r].second);
        }
    }

    if (diff == BotDifficulty::Ultra) {
        static int parity = 0;
        for (int i = 0; i < 10; ++i) {
            for (int j = 0; j < 10; ++j) {
                if ((i + j) % 2 == parity &&
                    cells[i][j].status != CellStatus::Miss &&
                    cells[i][j].status != CellStatus::Hit) {
                    return handleInputForBot(j, i);
                }
            }
        }
        parity = 1 - parity;
    }

    return handleSmartBotInput(BotDifficulty::Easy);
}

bool Field::canPlaceShip(int x, int y, int s, bool h) {
    for (int i = 0; i < s; i++) {
        int cx = h ? x + i : x;
        int cy = h ? y : y + i;
        if (cx >= 10 || cy >= 10) return false;
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                int nx = cx + dx, ny = cy + dy;
                if (nx >= 0 && nx < 10 && ny >= 0 && ny < 10 && cells[ny][nx].status == CellStatus::Ship)
                    return false;
            }
    }
    return true;
}

void Field::placeShipManually(int x, int y, int size, bool horizontal) {
    if (canPlaceShip(x, y, size, horizontal)) {
        ships.push_back(Ship(size, x, y, horizontal));
        for (int i = 0; i < size; ++i) {
            int cx = horizontal ? x + i : x;
            int cy = horizontal ? y : y + i;
            cells[cy][cx].status = CellStatus::Ship;
        }
    }
}

void Field::placeShipsRandomly() {
    clearField();
    int sizes[] = { 4,3,3,2,2,2,1,1,1,1 };
    for (int s : sizes) {
        bool placed = false;
        while (!placed) {
            int x = rand() % 10, y = rand() % 10;
            bool hor = rand() % 2 == 0;
            if (canPlaceShip(x, y, s, hor)) {
                ships.push_back(Ship(s, x, y, hor));
                for (int i = 0; i < s; i++) {
                    if (hor) cells[y][x + i].status = CellStatus::Ship;
                    else cells[y + i][x].status = CellStatus::Ship;
                }
                placed = true;
            }
        }
    }
}

void Field::undoLastShip(std::vector<int>& remainingShips) {
    if (ships.empty()) return;
    const Ship& lastShip = ships.back();
    for (const auto& part : lastShip.parts) {
        if (part.y >= 0 && part.y < 10 && part.x >= 0 && part.x < 10) {
            cells[part.y][part.x].status = CellStatus::Empty;
        }
    }
    if (lastShip.size >= 1 && lastShip.size <= 4) {
        remainingShips[lastShip.size - 1]++;
    }
    ships.pop_back();
}

void Field::useRadar(sf::Vector2i mPos, float oX, float oY) {
    int j = (mPos.x - (int)oX) / 40, i = (mPos.y - (int)oY) / 40;
    for (int r = i; r <= i + 1 && r < 10; r++)
        for (int c = j; c <= j + 1 && c < 10; c++)
            if (r >= 0 && c >= 0) cells[r][c].isIlluminated = true;
}

void Field::useSpotlight(int i, int j) {
    if (i < 0 || i >= 10 || j < 0 || j >= 10) return;
    for (auto& ship : ships) {
        bool found = false;
        for (auto& p : ship.parts) if (p.x == j && p.y == i) { found = true; break; }
        if (found) {
            for (auto& p : ship.parts)
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = p.x + dx, ny = p.y + dy;
                        if (nx >= 0 && nx < 10 && ny >= 0 && ny < 10)
                            cells[ny][nx].isIlluminated = true;
                    }
        }
    }
}

bool Field::allShipsDestroyed() {
    for (auto& s : ships) if (!s.isSunk()) return false;
    return true;
}

void Field::saveState() {
    backupCells = cells;
    backupShips = ships;
}

void Field::restoreState() {
    cells = backupCells;
    ships = backupShips;
}