#ifndef VECTOR_UTIL_H
#define VECTOR_UTIL_H

#include <SFML/System/Vector2.hpp>

template <typename T>
sf::Vector2<T> operator*(const sf::Vector2<T>& a, const sf::Vector2<T>& b) {
    return sf::Vector2<T>(a.x * b.x, a.y * b.y);
}

template <typename T>
sf::Vector2<T> operator/(const sf::Vector2<T>& a, const sf::Vector2<T>& b) {
    assert(b.x != 0);
    assert(b.y != 0);
    return sf::Vector2<T>(a.x / b.x, a.y / b.y);
}

#endif  // VECTOR_UTIL_H
