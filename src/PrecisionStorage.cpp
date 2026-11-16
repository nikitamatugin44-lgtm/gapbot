#include "PrecisionStorage.hpp"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <sstream>

void PrecisionStorage::setPosition(
    ObjectKey key,
    double x,
    double y
) {
    if (!std::isfinite(x) || !std::isfinite(y))
        return;

    m_positions[key] = {x, y};
}

bool PrecisionStorage::moveBy(
    ObjectKey key,
    double deltaX,
    double deltaY
) {
    if (!std::isfinite(deltaX) ||
        !std::isfinite(deltaY)) {
        return false;
    }

    auto it = m_positions.find(key);

    if (it == m_positions.end())
        return false;

    double newX = it->second.x + deltaX;
    double newY = it->second.y + deltaY;

    if (!std::isfinite(newX) ||
        !std::isfinite(newY)) {
        return false;
    }

    it->second.x = newX;
    it->second.y = newY;

    return true;
}

std::optional<PrecisionPosition>
PrecisionStorage::getPosition(ObjectKey key) const {
    auto it = m_positions.find(key);

    if (it == m_positions.end())
        return std::nullopt;

    return it->second;
}

bool PrecisionStorage::remove(ObjectKey key) {
    return m_positions.erase(key) > 0;
}

bool PrecisionStorage::contains(ObjectKey key) const {
    return m_positions.contains(key);
}

std::optional<std::pair<float, float>>
PrecisionStorage::toFloat(ObjectKey key) const {
    auto position = getPosition(key);

    if (!position)
        return std::nullopt;

    constexpr double maxFloat =
        std::numeric_limits<float>::max();

    if (std::abs(position->x) > maxFloat ||
        std::abs(position->y) > maxFloat) {
        return std::nullopt;
    }

    return std::pair<float, float>{
        static_cast<float>(position->x),
        static_cast<float>(position->y)
    };
}

std::optional<double> PrecisionStorage::parseNumber(
    const std::string& value
) {
    if (value.empty())
        return std::nullopt;

    errno = 0;

    char* end = nullptr;

    double result = std::strtod(
        value.c_str(),
        &end
    );

    if (end == value.c_str() || errno == ERANGE)
        return std::nullopt;

    while (*end == ' ' || *end == '\t' ||
           *end == '\n' || *end == '\r') {
        ++end;
    }

    if (*end != '\0' || !std::isfinite(result))
        return std::nullopt;

    return result;
}

std::string PrecisionStorage::formatNumber(double value) {
    if (!std::isfinite(value))
        return "invalid";

    std::ostringstream stream;

    stream << std::setprecision(
        std::numeric_limits<double>::max_digits10
    ) << value;

    return stream.str();
}

std::size_t PrecisionStorage::size() const {
    return m_positions.size();
}

void PrecisionStorage::clear() {
    m_positions.clear();
}
