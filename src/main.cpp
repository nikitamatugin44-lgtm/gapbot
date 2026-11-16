
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

#include <cmath>
#include <limits>

using namespace geode::prelude;

class $modify(PrecisionDiagnosticEditorUI, EditorUI) {

    bool init(LevelEditorLayer* layer) {
        if (!EditorUI::init(layer))
            return false;

        log::info("================================");
        log::info("Extended Precision 1.2");
        log::info("Precision diagnostics enabled");
        log::info("================================");

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

        // Position before movement
        const auto before = object->getPosition();

        // Let Geometry Dash perform its normal movement
        EditorUI::moveObject(object, offset);

        // Position after movement
        const auto after = object->getPosition();

        // Ignore calls without movement
        if (offset.x == 0.0f && offset.y == 0.0f)
            return;

        const float infinity =
            std::numeric_limits<float>::infinity();

        // Smallest representable float step near current position
        const float ulpX =
            std::nextafter(before.x, infinity) - before.x;

        const float ulpY =
            std::nextafter(before.y, infinity) - before.y;

        // Actual movement applied by Geometry Dash
        const double actualX =
            static_cast<double>(after.x) - before.x;

        const double actualY =
            static_cast<double>(after.y) - before.y;

        log::info(
            "[EP] Object ID: {}",
            object->m_uniqueID
        );

        log::info(
            "[EP] Input offset: X={:.12f} Y={:.12f}",
            static_cast<double>(offset.x),
            static_cast<double>(offset.y)
        );

        log::info(
            "[EP] Before: X={:.12f} Y={:.12f}",
            static_cast<double>(before.x),
            static_cast<double>(before.y)
        );

        log::info(
            "[EP] After: X={:.12f} Y={:.12f}",
            static_cast<double>(after.x),
            static_cast<double>(after.y)
        );

        log::info(
            "[EP] Actual delta: X={:.12f} Y={:.12f}",
            actualX,
            actualY
        );

        log::info(
            "[EP] Float ULP: X={:.12f} Y={:.12f}",
            static_cast<double>(ulpX),
            static_cast<double>(ulpY)
        );

        log::info("[EP] ------------------------------");
    }
};
