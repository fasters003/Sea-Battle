#pragma once
#include <SFML/Graphics.hpp>
#include <string>

class Button {
private:
    sf::RectangleShape shape;
    sf::Text text;
    sf::Font font;

public:
    Button(const sf::String& label, sf::Vector2f size, sf::Vector2f pos, sf::Color color);

    void draw(sf::RenderWindow& window);
    bool isClicked(sf::Vector2i mousePos);
};