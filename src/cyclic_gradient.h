#ifndef CYCLIC_GRADIENT_H
#define CYCLIC_GRADIENT_H

#include <cassert>
#include <vector>

#include <SFML/Graphics.hpp>

inline sf::Color lerp(const sf::Color& first, const sf::Color& second, double t) {
    assert(0 <= t);
    assert(t <= 1);
    const double r = static_cast<double>(first.r) * (1. - t) + static_cast<double>(second.r) * t;
    const double g = static_cast<double>(first.g) * (1. - t) + static_cast<double>(second.g) * t;
    const double b = static_cast<double>(first.b) * (1. - t) + static_cast<double>(second.b) * t;
    const double a = static_cast<double>(first.a) * (1. - t) + static_cast<double>(second.a) * t;
    return sf::Color(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b),
                     static_cast<uint8_t>(a));
}

class CyclicGradient {
public:
    static const CyclicGradient DEFAULT_GRADIENT;

    CyclicGradient(double length, const std::vector<std::pair<double, sf::Color>>& curve);

    sf::Color operator()(double val) const;

private:
    static constexpr auto cmp = [](const std::pair<double, sf::Color>& a,
                                   const std::pair<double, sf::Color>& b) {
        return a.first < b.first;
    };

    double mLength;
    std::vector<std::pair<double, sf::Color>> mKeyframes;
};

#endif  // CYCLIC_GRADIENT_H
