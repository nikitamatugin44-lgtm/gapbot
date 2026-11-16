#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

struct PrecisionPosition {
    double x = 0.0;
    double y = 0.0;
};

class PrecisionStorage {
public:
    using ObjectKey = std::uint64_t;

    // Create or replace exact coordinates.
    void setPosition(
        ObjectKey key,
        double x,
        double y
    );

    // Add movement without converting to float.
    bool moveBy(
        ObjectKey key,
        double deltaX,
        double deltaY
    );

    // Retrieve exact coordinates.
    std::optional<PrecisionPosition> getPosition(
        ObjectKey key
    ) const;

    // Remove an object's coordinates.
    bool remove(ObjectKey key);

    // Check whether an object exists.
    bool contains(ObjectKey key) const;

    // Convert only when communicating with the game engine.
    std::optional<std::pair<float, float>> toFloat(
        ObjectKey key
    ) const;

    // Parse manually entered coordinates.
    static std::optional<double> parseNumber(
        const std::string& value
    );

    // Format a double without losing round-trip precision.
    static std::string formatNumber(double value);

    std::size_t size() const;

    void clear();

private:
    std::unordered_map<ObjectKey, PrecisionPosition> m_positions;
};
