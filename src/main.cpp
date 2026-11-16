
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

#include <unordered_map>
#include <cstdint>
#include <cmath>

using namespace geode::prelude;

struct ExtendedPosition {
    double x = 0.0;
    double y = 0.0;
};

class $modify(PrecisionEditorUI, EditorUI) {

    static inline std::unordered_map<
        GameObject*,
        ExtendedPosition
    > s_positions;

    bool init(LevelEditorLayer* layer) {
        if (!EditorUI::init(layer))
            return false;

        // New editor session.
        s_positions.clear();

        log::info("Extended Precision: Editor connected");

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

        // Initialize the precise position only once.
        auto [it, inserted] = s_positions.try_emplace(
            object,
            ExtendedPosition{
                static_cast<double>(object->getPositionX()),
                static_cast<double>(object->getPositionY())
            }
        );

        auto& precise = it->second;

        // Let Geometry Dash perform its normal movement.
        EditorUI::moveObject(object, offset);

        // Accumulate the original movement in double.
        precise.x += static_cast<double>(offset.x);
        precise.y += static_cast<double>(offset.y);

        // Reject invalid coordinates.
        if (!std::isfinite(precise.x) ||
            !std::isfinite(precise.y)) {
            s_positions.erase(object);
            return;
        }

        // Convert only at the engine boundary.
        object->setPosition({
            static_cast<float>(precise.x),
            static_cast<float>(precise.y)
        });
    }
};
