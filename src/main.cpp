
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

#include <unordered_map>
#include <cmath>

using namespace geode::prelude;

struct PrecisionAccumulator {
    double x = 0.0;
    double y = 0.0;

    float lastX = 0.0f;
    float lastY = 0.0f;
};

class $modify(PrecisionEditorUI, EditorUI) {

    static inline std::unordered_map<
        GameObject*,
        PrecisionAccumulator
    > s_accumulators;

    bool init(LevelEditorLayer* layer) {
        if (!EditorUI::init(layer))
            return false;

        s_accumulators.clear();

        log::info("[EP] Double Accumulator 1.3 initialized");

        return true;
    }

    void moveObject(
        GameObject* object,
        cocos2d::CCPoint offset
    ) {
        if (!object) {
            EditorUI::moveObject(object, offset);
            return;
        }

        auto& acc = s_accumulators[object];

        auto before = object->getPosition();

        // Detect movement performed outside this accumulator.
        if (acc.lastX != before.x || acc.lastY != before.y) {
            acc.x = 0.0;
            acc.y = 0.0;
        }

        // First movement initializes the stored position.
        if (acc.lastX == 0.0f && acc.lastY == 0.0f) {
            acc.lastX = before.x;
            acc.lastY = before.y;
        }

        // Accumulate input in double precision.
        acc.x += static_cast<double>(offset.x);
        acc.y += static_cast<double>(offset.y);

        // Calculate representable target positions.
        const float targetX = static_cast<float>(
            static_cast<double>(before.x) + acc.x
        );

        const float targetY = static_cast<float>(
            static_cast<double>(before.y) + acc.y
        );

        // Only send movement when float can represent it.
        const float moveX = targetX != before.x
            ? targetX - before.x
            : 0.0f;

        const float moveY = targetY != before.y
            ? targetY - before.y
            : 0.0f;

        // No representable movement yet.
        if (moveX == 0.0f && moveY == 0.0f) {
            acc.lastX = before.x;
            acc.lastY = before.y;

            log::info(
                "[EP] Accumulating: X={:.12f} Y={:.12f}",
                acc.x,
                acc.y
            );

            return;
        }

        // Apply accumulated movement.
        EditorUI::moveObject(
            object,
            cocos2d::CCPoint(moveX, moveY)
        );

        auto after = object->getPosition();

        // Clear only successfully applied axes.
        if (after.x != before.x)
            acc.x = 0.0;

        if (after.y != before.y)
            acc.y = 0.0;

        acc.lastX = after.x;
        acc.lastY = after.y;

        log::info(
            "[EP] Applied: X={:.12f} Y={:.12f}",
            static_cast<double>(after.x) - before.x,
            static_cast<double>(after.y) - before.y
        );

        log::info(
            "[EP] Remaining: X={:.12f} Y={:.12f}",
            acc.x,
            acc.y
        );
    }
};
