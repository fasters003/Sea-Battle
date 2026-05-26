#include "Button.h"

Button::Button(const sf::String& label, sf::Vector2f size, sf::Vector2f pos, sf::Color color) {
    if (!font.loadFromFile("arial.ttf")) {
    }
    shape.setSize(size);
    shape.setPosition(pos);
    shape.setFillColor(color);

    text.setFont(font);
    text.setString(label);
    text.setCharacterSize(18);
    text.setFillColor(sf::Color::White);

    sf::FloatRect textRect = text.getLocalBounds();
    text.setOrigin(textRect.left + textRect.width / 2.0f,
        textRect.top + textRect.height / 2.0f);
    text.setPosition(pos.x + size.x / 2.0f, pos.y + size.y / 2.0f);
}

void Button::draw(sf::RenderWindow& window) {
    window.draw(shape);
    window.draw(text);
}

bool Button::isClicked(sf::Vector2i mousePos) {
    return shape.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos));
}