#include <SFML/Graphics.hpp>
#include "Field.h"
#include "Button.h"
#include <ctime>
#include <string>
#include <vector>
#include <fstream>

void saveMatchToHistory(const sf::String& name, BotDifficulty diff, bool isWin, int destroyed, int lost, int timeSec) {
    std::ofstream out("stats.txt", std::ios::app);
    if (!out.is_open()) return;

    sf::String safeName = name;
    for (std::size_t i = 0; i < safeName.getSize(); ++i) {
        if (safeName[i] == L' ') safeName[i] = L'_';
    }
    if (safeName.isEmpty()) safeName = L"Unknown";

    std::basic_string<sf::Uint8> u8 = safeName.toUtf8();
    std::string utf8String(u8.begin(), u8.end());

    std::string diffStr;
    if (diff == BotDifficulty::Easy) diffStr = "EASY";
    else if (diff == BotDifficulty::Hard) diffStr = "HARD";
    else diffStr = "ULTRA";

    std::string resStr = isWin ? "WIN" : "LOSS";

    out << utf8String << " " << diffStr << " " << resStr << " "
        << destroyed << " " << lost << " " << timeSec << "\n";
    out.close();
}

sf::String getMatchHistoryString() {
    std::vector<sf::String> history;
    std::ifstream in("stats.txt");
    if (!in.is_open()) {
        return L"История пуста. Сыграйте первый бой!";
    }

    std::string u8Name, diff, res;
    int dest, lost, t;

    while (in >> u8Name >> diff >> res >> dest >> lost >> t) {
        sf::String name = sf::String::fromUtf8(u8Name.begin(), u8Name.end());

        for (std::size_t i = 0; i < name.getSize(); ++i) {
            if (name[i] == L'_') name[i] = L' ';
        }

        sf::String diffW = (diff == "EASY") ? L"ЛЕГКО" : (diff == "HARD" ? L"НОРМАЛЬНО" : L"УЛЬТРА");
        sf::String resW = (res == "WIN") ? L"ПОБЕДА" : L"ПОРАЖЕНИЕ";

        sf::String line = L"Игрок: " + name +
            L"  |  Сложность: " + diffW +
            L"  |  Итог: " + resW +
            L"  |  Счёт: " + std::to_wstring(dest) + L"-" + std::to_wstring(lost) +
            L"  |  Время: " + std::to_wstring(t) + L"с";
        history.push_back(line);
    }
    in.close();

    if (history.empty()) return L"История пуста. Сыграйте первый бой!";

    size_t start = 0;
    if (history.size() > 15) start = history.size() - 15;

    sf::String fullText = L"";
    for (size_t i = start; i < history.size(); ++i) {
        fullText += history[i] + L"\n\n";
    }
    return fullText;
}

enum class GameState { NameInput, Menu, ShipPlacement, Playing, GameOver, StatsView };
enum class ClickMode { Shoot, Radar, Spotlight, DoubleShot };

int main() {
    sf::RenderWindow window(sf::VideoMode(1200, 820), L"Морской Бой");
    window.setFramerateLimit(60);

    srand(static_cast<unsigned int>(time(0)));

    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {}

    sf::Text uiText, hintText, remainingText, titleText, historyText;
    uiText.setFont(font);      uiText.setCharacterSize(22); uiText.setFillColor(sf::Color::Yellow);
    hintText.setFont(font);    hintText.setCharacterSize(24); hintText.setFillColor(sf::Color::White);
    remainingText.setFont(font); remainingText.setCharacterSize(20); remainingText.setFillColor(sf::Color::Cyan);

    historyText.setFont(font);
    historyText.setCharacterSize(22);
    historyText.setFillColor(sf::Color::White);

    sf::Text resultText;
    resultText.setFont(font);
    resultText.setCharacterSize(60);
    resultText.setStyle(sf::Text::Bold);

    titleText.setFont(font);
    titleText.setCharacterSize(80);
    titleText.setFillColor(sf::Color::White);
    titleText.setString(L"МОРСКОЙ БОЙ");

    sf::Text nameInputText, statText;
    nameInputText.setFont(font);
    nameInputText.setCharacterSize(40);
    nameInputText.setFillColor(sf::Color::Yellow);
    statText.setFont(font);
    statText.setCharacterSize(30);
    statText.setFillColor(sf::Color::White);

    Field pField, eField;
    GameState state = GameState::NameInput;
    ClickMode curMode = ClickMode::Shoot;
    BotDifficulty botDiff = BotDifficulty::Hard;

    bool pTurn = true;
    bool shipHorizontal = true;
    int selectedSize = 1;
    int doubleShotsRemaining = 0;
    int shipsDestroyedByPlayer = 0;

    sf::String playerName = L"";
    int shipsDestroyedByEnemy = 0;

    sf::Clock battleClock;
    int battleTimeSeconds = 0;

    sf::Clock botTimer;
    sf::Clock cursorClock;
    bool showCursor = true;

    std::vector<int> backupRemainingShips;
    bool canUndoRandom = false;
    std::vector<int> remainingShips = { 4, 3, 2, 1 };

    Button bEasy(L"ЛЕГКО", { 220, 60 }, { 490, 280 }, sf::Color(70, 150, 70));
    Button bHard(L"НОРМАЛЬНО", { 220, 60 }, { 490, 360 }, sf::Color(150, 110, 50));
    Button bUltra(L"УЛЬТРА", { 220, 60 }, { 490, 440 }, sf::Color(180, 50, 50));

    Button bViewStats(L"ИСТОРИЯ БОЁВ", { 220, 60 }, { 490, 520 }, sf::Color(100, 150, 200));
    Button bBack(L"НАЗАД", { 220, 60 }, { 490, 720 }, sf::Color(180, 70, 70));

    Button bShip1(L"1", { 50, 50 }, { 820, 150 }, sf::Color(80, 120, 200));
    Button bShip2(L"2", { 50, 50 }, { 820, 220 }, sf::Color(80, 120, 200));
    Button bShip3(L"3", { 50, 50 }, { 820, 290 }, sf::Color(80, 120, 200));
    Button bShip4(L"4", { 50, 50 }, { 820, 360 }, sf::Color(80, 120, 200));

    Button bRotate(L"R - Поворот", { 210, 50 }, { 820, 440 }, sf::Color(100, 100, 180));
    Button bRandom(L"СЛУЧАЙНО", { 210, 50 }, { 820, 510 }, sf::Color(100, 100, 220));
    Button bReady(L"В БОЙ", { 210, 50 }, { 820, 580 }, sf::Color(50, 200, 80));
    Button bUndo(L"ОТМЕНА", { 210, 50 }, { 820, 650 }, sf::Color(180, 70, 70));
    Button bUndoRandom(L"ВЕРНУТЬ", { 210, 50 }, { 820, 720 }, sf::Color(180, 120, 50));

    Button bRdr(L"Радар (2)", { 115, 45 }, { 50, 650 }, sf::Color(70, 70, 150));
    Button bSpt(L"Подсвет (1)", { 115, 45 }, { 175, 650 }, sf::Color(150, 70, 70));
    Button bDbl(L"Двойной (2)", { 115, 45 }, { 300, 650 }, sf::Color(150, 150, 70));

    Button bMenu(L"ГЛАВНОЕ МЕНЮ", { 220, 60 }, { 490, 600 }, sf::Color(100, 100, 200));

    auto triggerGameOver = [&](bool isWin) {
        state = GameState::GameOver;
        battleTimeSeconds = static_cast<int>(battleClock.getElapsedTime().asSeconds());

        resultText.setString(isWin ? L"ВЫ ПОБЕДИЛИ!" : L"ВЫ ПРОИГРАЛИ!");
        resultText.setFillColor(isWin ? sf::Color::Green : sf::Color::Red);

        saveMatchToHistory(playerName, botDiff, isWin, shipsDestroyedByPlayer, shipsDestroyedByEnemy, battleTimeSeconds);

        statText.setString(
            L"Игрок: " + playerName + L"\n\n" +
            L"Время боя: " + std::to_wstring(battleTimeSeconds / 60) + L"м " + std::to_wstring(battleTimeSeconds % 60) + L"с\n" +
            L"Уничтожено кораблей врага: " + std::to_wstring(shipsDestroyedByPlayer) + L" / 10\n" +
            L"Потеряно ваших кораблей: " + std::to_wstring(shipsDestroyedByEnemy) + L" / 10"
        );
        sf::FloatRect statRect = statText.getLocalBounds();
        statText.setOrigin(statRect.left + statRect.width / 2.0f, statRect.top + statRect.height / 2.0f);
        statText.setPosition(600, 420);
        };

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();

            if (state == GameState::NameInput && event.type == sf::Event::TextEntered) {
                showCursor = true;
                cursorClock.restart();

                if (event.text.unicode == 8) {
                    if (playerName.getSize() > 0) playerName.erase(playerName.getSize() - 1, 1);
                }
                else if (event.text.unicode == 13) {
                    if (playerName.getSize() > 0) state = GameState::Menu;
                }
                else if (event.text.unicode >= 32 && playerName.getSize() < 15) {
                    playerName += event.text.unicode;
                }
            }

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2i m = sf::Mouse::getPosition(window);

                if (state == GameState::Menu) {
                    if (bEasy.isClicked(m)) { botDiff = BotDifficulty::Easy; state = GameState::ShipPlacement; pField.clearField(); remainingShips = { 4,3,2,1 }; canUndoRandom = false; }
                    if (bHard.isClicked(m)) { botDiff = BotDifficulty::Hard; state = GameState::ShipPlacement; pField.clearField(); remainingShips = { 4,3,2,1 }; canUndoRandom = false; }
                    if (bUltra.isClicked(m)) { botDiff = BotDifficulty::Ultra; state = GameState::ShipPlacement; pField.clearField(); remainingShips = { 4,3,2,1 }; canUndoRandom = false; }

                    if (bViewStats.isClicked(m)) {
                        state = GameState::StatsView;
                        historyText.setString(getMatchHistoryString());
                    }
                }
                else if (state == GameState::StatsView) {
                    if (bBack.isClicked(m)) {
                        state = GameState::Menu;
                    }
                }
                else if (state == GameState::ShipPlacement) {
                    if (bShip1.isClicked(m)) selectedSize = 1;
                    if (bShip2.isClicked(m)) selectedSize = 2;
                    if (bShip3.isClicked(m)) selectedSize = 3;
                    if (bShip4.isClicked(m)) selectedSize = 4;

                    if (bRandom.isClicked(m)) {
                        pField.saveState();
                        backupRemainingShips = remainingShips;
                        canUndoRandom = true;

                        pField.placeShipsRandomly();
                        remainingShips = { 0,0,0,0 };
                    }
                    else if (bUndoRandom.isClicked(m) && canUndoRandom) {
                        pField.restoreState();
                        remainingShips = backupRemainingShips;
                        canUndoRandom = false;
                    }
                    else if (bReady.isClicked(m) && pField.getShipsCount() == 10) {
                        state = GameState::Playing;
                        eField.placeShipsRandomly();
                        pTurn = true;
                        pField.abilityPoints = 0;
                        shipsDestroyedByPlayer = 0;
                        shipsDestroyedByEnemy = 0;
                        curMode = ClickMode::Shoot;
                        battleClock.restart();
                    }
                    else if (bUndo.isClicked(m) && pField.getShipsCount() > 0) {
                        pField.undoLastShip(remainingShips);
                        canUndoRandom = false;
                    }
                    else {
                        int x = (m.x - 100) / 40;
                        int y = (m.y - 120) / 40;
                        if (x >= 0 && x < 10 && y >= 0 && y < 10) {
                            if (selectedSize >= 1 && selectedSize <= 4 && remainingShips[selectedSize - 1] > 0) {
                                if (pField.canPlaceShip(x, y, selectedSize, shipHorizontal)) {
                                    pField.placeShipManually(x, y, selectedSize, shipHorizontal);
                                    remainingShips[selectedSize - 1]--;
                                    canUndoRandom = false;
                                }
                            }
                        }
                    }
                }
                else if (state == GameState::Playing && pTurn) {
                    if (bRdr.isClicked(m) && pField.abilityPoints >= 2) {
                        curMode = ClickMode::Radar;
                    }
                    else if (bSpt.isClicked(m) && pField.abilityPoints >= 1) {
                        curMode = ClickMode::Spotlight;
                    }
                    else if (bDbl.isClicked(m) && pField.abilityPoints >= 2) {
                        curMode = ClickMode::DoubleShot;
                        doubleShotsRemaining = 2;
                    }
                    else {
                        int x = (m.x - 680) / 40;
                        int y = (m.y - 120) / 40;

                        if (x >= 0 && x < 10 && y >= 0 && y < 10) {
                            if (curMode == ClickMode::Shoot) {
                                int result = eField.handleInputForBot(x, y);

                                if (result == -1) {
                                    pTurn = false;
                                    botTimer.restart();
                                }
                                else if (result > 0) {
                                    shipsDestroyedByPlayer++;
                                    if (shipsDestroyedByPlayer % 2 == 0) {
                                        pField.abilityPoints += 1;
                                    }

                                    if (eField.allShipsDestroyed()) {
                                        triggerGameOver(true);
                                    }
                                }
                            }
                            else if (curMode == ClickMode::Radar) {
                                eField.useRadar(m, 680, 120);
                                pField.abilityPoints -= 2;
                                curMode = ClickMode::Shoot;
                            }
                            else if (curMode == ClickMode::Spotlight) {
                                eField.useSpotlight(y, x);
                                pField.abilityPoints -= 1;
                                curMode = ClickMode::Shoot;
                            }
                            else if (curMode == ClickMode::DoubleShot) {
                                int result = eField.handleInputForBot(x, y);
                                doubleShotsRemaining--;

                                if (result > 0) {
                                    shipsDestroyedByPlayer++;
                                    if (shipsDestroyedByPlayer % 2 == 0) {
                                        pField.abilityPoints += 1;
                                    }

                                    if (eField.allShipsDestroyed()) {
                                        triggerGameOver(true);
                                    }
                                }

                                if (doubleShotsRemaining <= 0) {
                                    pField.abilityPoints -= 2;
                                    curMode = ClickMode::Shoot;
                                    if (result == -1) {
                                        pTurn = false;
                                        botTimer.restart();
                                    }
                                }
                            }
                        }
                    }
                }
                else if (state == GameState::GameOver) {
                    if (bMenu.isClicked(m)) {
                        state = GameState::Menu;
                        pField.clearField();
                        eField.clearField();
                        curMode = ClickMode::Shoot;
                        pTurn = true;
                        shipsDestroyedByPlayer = 0;
                        shipsDestroyedByEnemy = 0;
                    }
                }
            }

            if (event.type == sf::Event::KeyPressed && state == GameState::ShipPlacement) {
                if (event.key.code == sf::Keyboard::R) shipHorizontal = !shipHorizontal;
                if (event.key.code == sf::Keyboard::BackSpace || event.key.code == sf::Keyboard::Delete) {
                    if (pField.getShipsCount() > 0) {
                        pField.undoLastShip(remainingShips);
                        canUndoRandom = false;
                    }
                }
            }
        }

        if (state == GameState::NameInput) {
            if (cursorClock.getElapsedTime().asMilliseconds() >= 500) {
                showCursor = !showCursor;
                cursorClock.restart();
            }
        }

        if (state == GameState::Playing && !pTurn) {
            if (botTimer.getElapsedTime().asMilliseconds() >= 500) {
                int result = pField.handleSmartBotInput(botDiff);

                if (result == -1) {
                    pTurn = true;
                    curMode = ClickMode::Shoot;
                }
                else if (result > 0) {
                    shipsDestroyedByEnemy++;
                    if (pField.allShipsDestroyed()) {
                        triggerGameOver(false);
                    }
                }

                botTimer.restart();
            }
        }

        window.clear(sf::Color(25, 25, 35));

        if (state == GameState::NameInput) {
            titleText.setString(L"ВВЕДИТЕ ВАШЕ ИМЯ");
            sf::FloatRect tRect = titleText.getLocalBounds();
            titleText.setOrigin(tRect.left + tRect.width / 2.0f, tRect.top + tRect.height / 2.0f);
            titleText.setPosition(600, 300);
            window.draw(titleText);

            nameInputText.setString(playerName + (showCursor ? L"|" : L" "));
            sf::FloatRect nRect = nameInputText.getLocalBounds();
            nameInputText.setOrigin(nRect.left + nRect.width / 2.0f, nRect.top + nRect.height / 2.0f);
            nameInputText.setPosition(600, 400);
            window.draw(nameInputText);

            hintText.setString(L"Нажмите ENTER для продолжения");
            sf::FloatRect hRect = hintText.getLocalBounds();
            hintText.setOrigin(hRect.left + hRect.width / 2.0f, hRect.top + hRect.height / 2.0f);
            hintText.setPosition(600, 500);
            window.draw(hintText);
        }
        else if (state == GameState::Menu) {
            titleText.setString(L"МОРСКОЙ БОЙ");
            sf::FloatRect titleRect = titleText.getLocalBounds();
            titleText.setOrigin(titleRect.left + titleRect.width / 2.0f, titleRect.top + titleRect.height / 2.0f);
            titleText.setPosition(600, 180);
            window.draw(titleText);

            bEasy.draw(window);
            bHard.draw(window);
            bUltra.draw(window);
            bViewStats.draw(window);
        }
        else if (state == GameState::StatsView) {
            titleText.setString(L"ИСТОРИЯ БОЁВ");
            sf::FloatRect titleRect = titleText.getLocalBounds();
            titleText.setOrigin(titleRect.left + titleRect.width / 2.0f, titleRect.top + titleRect.height / 2.0f);
            titleText.setPosition(600, 100);
            window.draw(titleText);

            historyText.setPosition(100, 180);
            window.draw(historyText);

            bBack.draw(window);
        }
        else if (state == GameState::ShipPlacement) {
            pField.draw(window, 100, 120, false);

            sf::Vector2i mouse = sf::Mouse::getPosition(window);
            int gx = (mouse.x - 100) / 40;
            int gy = (mouse.y - 120) / 40;

            if (gx >= 0 && gx < 10 && gy >= 0 && gy < 10 && selectedSize >= 1 && selectedSize <= 4 && remainingShips[selectedSize - 1] > 0) {
                sf::RectangleShape preview(sf::Vector2f(38.f, 38.f));
                preview.setFillColor(sf::Color(255, 255, 100, 130));
                preview.setOutlineColor(sf::Color::Yellow);
                preview.setOutlineThickness(2.f);

                bool canPlace = pField.canPlaceShip(gx, gy, selectedSize, shipHorizontal);
                for (int i = 0; i < selectedSize; ++i) {
                    int px = shipHorizontal ? gx + i : gx;
                    int py = shipHorizontal ? gy : gy + i;
                    if (px < 10 && py < 10) {
                        preview.setPosition(100 + px * 40.f, 120 + py * 40.f);
                        if (canPlace) window.draw(preview);
                    }
                }
            }

            hintText.setString(L"Игрок: " + playerName + L" | ЛКМ - Поставить | R - Поворот | Backspace - Отмена");
            hintText.setPosition(280, 30);
            window.draw(hintText);

            remainingText.setString(
                L"Осталось кораблей:\n\n"
                L"1-палубные : " + std::to_wstring(remainingShips[0]) + L"\n"
                L"2-палубные : " + std::to_wstring(remainingShips[1]) + L"\n"
                L"3-палубные : " + std::to_wstring(remainingShips[2]) + L"\n"
                L"4-палубные : " + std::to_wstring(remainingShips[3])
            );
            remainingText.setPosition(600, 150);
            window.draw(remainingText);

            bShip1.draw(window); bShip2.draw(window); bShip3.draw(window); bShip4.draw(window);
            bRotate.draw(window); bRandom.draw(window); bReady.draw(window); bUndo.draw(window);

            if (canUndoRandom) {
                bUndoRandom.draw(window);
            }
        }
        else if (state == GameState::Playing) {
            pField.draw(window, 80, 120, false);
            eField.draw(window, 680, 120, true);

            bRdr.draw(window);
            bSpt.draw(window);
            bDbl.draw(window);

            sf::String modeStr;
            switch (curMode) {
            case ClickMode::Shoot:     modeStr = L"СТРЕЛЬБА"; break;
            case ClickMode::Radar:     modeStr = L"РАДАР"; break;
            case ClickMode::Spotlight: modeStr = L"ПОДСВЕТКА"; break;
            case ClickMode::DoubleShot:modeStr = L"ДВОЙНОЙ (" + std::to_wstring(doubleShotsRemaining) + L")"; break;
            }

            uiText.setString(L"Игрок: " + playerName + L" | AP: " + std::to_wstring(pField.abilityPoints) +
                L" | Режим: " + modeStr +
                (pTurn ? L" | ВАШ ХОД" : L" | ХОД БОТА"));
            uiText.setPosition(80, 40);
            window.draw(uiText);
        }
        else if (state == GameState::GameOver) {
            pField.draw(window, 80, 120, false);
            eField.draw(window, 680, 120, true);

            sf::RectangleShape overlay(sf::Vector2f(1200, 820));
            overlay.setFillColor(sf::Color(0, 0, 0, 200));
            window.draw(overlay);

            sf::FloatRect textRect = resultText.getLocalBounds();
            resultText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
            resultText.setPosition(600, 250);
            window.draw(resultText);

            statText.setString(
                L"Игрок: " + playerName + L"\n\n" +
                L"Время боя: " + std::to_wstring(battleTimeSeconds / 60) + L"м " + std::to_wstring(battleTimeSeconds % 60) + L"с\n" +
                L"Уничтожено кораблей врага: " + std::to_wstring(shipsDestroyedByPlayer) + L" / 10\n" +
                L"Потеряно ваших кораблей: " + std::to_wstring(shipsDestroyedByEnemy) + L" / 10"
            );
            sf::FloatRect statRect = statText.getLocalBounds();
            statText.setOrigin(statRect.left + statRect.width / 2.0f, statRect.top + statRect.height / 2.0f);
            statText.setPosition(600, 420);
            window.draw(statText);

            bMenu.draw(window);
        }

        window.display();
    }
    return 0;
}