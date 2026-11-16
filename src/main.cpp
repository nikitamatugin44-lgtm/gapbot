
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

class $modify(PrecisionEditorUI, EditorUI) {

    static inline bool s_diagnosticsEnabled = false;
    static inline std::string s_lastDiagnostic =
        "No movement data yet.";

    bool init(LevelEditorLayer* layer) {
        if (!EditorUI::init(layer))
            return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        // Main EP button
        auto epSprite = ButtonSprite::create("EP");

        auto epButton = CCMenuItemSpriteExtra::create(
            epSprite,
            this,
            menu_selector(PrecisionEditorUI::onOpenEP)
        );

        // Diagnostics toggle
        auto diagSprite = ButtonSprite::create("DIAG");

        auto diagButton = CCMenuItemSpriteExtra::create(
            diagSprite,
            this,
            menu_selector(PrecisionEditorUI::onToggleDiagnostics)
        );

        auto menu = CCMenu::create();

        epButton->setPosition(0, 0);
        diagButton->setPosition(0, -42);

        menu->addChild(epButton);
        menu->addChild(diagButton);

        menu->setPosition(42, winSize.height - 170);
        menu->setID("extended-precision-menu"_spr);

        this->addChild(menu, 9999);

        log::info("[EP] In-game menu initialized");

        return true;
    }

    void onOpenEP(CCObject*) {
        std::string objectInfo = "No object selected.";

        if (m_selectedObject) {
            auto pos = m_selectedObject->getPosition();

            objectInfo = fmt::format(
                "Selected Object\n"
                "ID: {}\n"
                "X: {:.12f}\n"
                "Y: {:.12f}\n",
                m_selectedObject->m_uniqueID,
                static_cast<double>(pos.x),
                static_cast<double>(pos.y)
            );
        }

        std::string body = fmt::format(
            "EXTENDED PRECISION 1.4\n\n"
            "Diagnostics: {}\n\n"
            "{}\n"
            "Last movement:\n{}",
            s_diagnosticsEnabled ? "ON" : "OFF",
            objectInfo,
            s_lastDiagnostic
        );

        FLAlertLayer::create(
            "Extended Precision",
            body,
            "Close"
        )->show();
    }

    void onToggleDiagnostics(CCObject*) {
        s_diagnosticsEnabled = !s_diagnosticsEnabled;

        log::info(
            "[EP] Diagnostics: {}",
            s_diagnosticsEnabled ? "ON" : "OFF"
        );

        FLAlertLayer::create(
            "Extended Precision",
            s_diagnosticsEnabled
                ? "Diagnostics enabled!"
                : "Diagnostics disabled!",
            "OK"
        )->show();
    }

    void moveObject(
        GameObject* object,
        cocos2d::CCPoint offset
    ) {
        if (!object) {
            EditorUI::moveObject(object, offset);
            return;
        }

        auto before = object->getPosition();

        EditorUI::moveObject(object, offset);

        auto after = object->getPosition();

        if (!s_diagnosticsEnabled)
            return;

        s_lastDiagnostic = fmt::format(
            "Object ID: {}\n"
            "Input X: {:.12f}\n"
            "Input Y: {:.12f}\n"
            "Actual X: {:.12f}\n"
            "Actual Y: {:.12f}",
            object->m_uniqueID,
            static_cast<double>(offset.x),
            static_cast<double>(offset.y),
            static_cast<double>(after.x) - before.x,
            static_cast<double>(after.y) - before.y
        );

        log::info("[EP] {}", s_lastDiagnostic);
    }
};
