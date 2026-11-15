#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/modify/EditorUI.hpp>

#include <string>
#include <unordered_map>
#include <utility>
#include <cstdlib>
#include <cmath>
#include <cstdio>

using namespace geode::prelude;

// ==========================================
// EXTENDED PRECISION STORAGE
// ==========================================

struct ExactPosition {
    std::string x;
    std::string y;
};

static std::unordered_map<GameObject*, ExactPosition> g_positions;

static std::string formatFloat(float value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.9g", value);
    return buffer;
}

static bool parseNumber(const std::string& text, double& result) {
    if (text.empty())
        return false;

    char* end = nullptr;
    result = std::strtod(text.c_str(), &end);

    return end != text.c_str()
        && *end == '\0'
        && std::isfinite(result);
}

// ==========================================
// PRECISION EDITOR
// ==========================================

class PrecisionPopup : public Popup {
protected:
    GameObject* m_object = nullptr;

    TextInput* m_xInput = nullptr;
    TextInput* m_yInput = nullptr;

    CCLabelBMFont* m_status = nullptr;

    void setStatus(const char* text) {
        if (m_status)
            m_status->setString(text);
    }

    void onLoad(CCObject*) {
        if (!m_object)
            return;

        auto it = g_positions.find(m_object);

        if (it != g_positions.end()) {
            m_xInput->setString(it->second.x);
            m_yInput->setString(it->second.y);
        }
        else {
            m_xInput->setString(
                formatFloat(m_object->getPositionX())
            );

            m_yInput->setString(
                formatFloat(m_object->getPositionY())
            );
        }

        setStatus("Coordinates loaded");
    }

    void onApply(CCObject*) {
        if (!m_object) {
            setStatus("Object unavailable");
            return;
        }

        std::string xText = m_xInput->getString();
        std::string yText = m_yInput->getString();

        double x = 0.0;
        double y = 0.0;

        if (!parseNumber(xText, x) ||
            !parseNumber(yText, y)) {
            setStatus("Invalid coordinates");
            return;
        }

        // Preserve the original decimal strings.
        g_positions[m_object] = {
            xText,
            yText
        };

        // Cocos2d-x still receives float coordinates.
        m_object->setPosition(
            ccp(
                static_cast<float>(x),
                static_cast<float>(y)
            )
        );

        setStatus("Applied (engine float precision)");
    }

    bool init(GameObject* object) {
        if (!Popup::init(340.f, 220.f))
            return false;

        if (!object)
            return false;

        m_object = object;

        this->setTitle("Extended Precision");

        auto makeLabel = [&](const char* text, float x, float y) {
            auto label = CCLabelBMFont::create(
                text,
                "bigFont.fnt"
            );

            label->setScale(0.45f);

            m_mainLayer->addChildAtPosition(
                label,
                Anchor::Center,
                ccp(x, y)
            );

            return label;
        };

        makeLabel("X POSITION", -100.f, 75.f);
        makeLabel("Y POSITION",  100.f, 75.f);

        m_xInput = TextInput::create(
            145.f,
            "X coordinate",
            "bigFont.fnt"
        );

        m_yInput = TextInput::create(
            145.f,
            "Y coordinate",
            "bigFont.fnt"
        );

        m_xInput->setFilter("0123456789.-");
        m_yInput->setFilter("0123456789.-");

        m_xInput->setScale(0.75f);
        m_yInput->setScale(0.75f);

        m_mainLayer->addChildAtPosition(
            m_xInput,
            Anchor::Center,
            ccp(-90.f, 40.f)
        );

        m_mainLayer->addChildAtPosition(
            m_yInput,
            Anchor::Center,
            ccp(90.f, 40.f)
        );

        m_status = makeLabel(
            "Ready",
            0.f,
            -5.f
        );

        auto loadButton = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("LOAD"),
            this,
            menu_selector(PrecisionPopup::onLoad)
        );

        auto applyButton = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("APPLY"),
            this,
            menu_selector(PrecisionPopup::onApply)
        );

        m_buttonMenu->addChildAtPosition(
            loadButton,
            Anchor::Center,
            ccp(-75.f, -65.f)
        );

        m_buttonMenu->addChildAtPosition(
            applyButton,
            Anchor::Center,
            ccp(75.f, -65.f)
        );

        onLoad(nullptr);

        return true;
    }

public:
    static PrecisionPopup* create(GameObject* object) {
        auto ret = new PrecisionPopup();

        if (ret->init(object)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};

// ==========================================
// EDITOR INTEGRATION
// ==========================================

class $modify(PrecisionEditorUI, EditorUI) {
    bool init(LevelEditorLayer* layer) {
        if (!EditorUI::init(layer))
            return false;

        if (!Mod::get()->getSettingValue<bool>("enabled"))
            return true;

        auto win = CCDirector::get()->getWinSize();

        auto sprite = ButtonSprite::create(
            "PRECISION",
            90,
            30,
            0.55f,
            false,
            "goldFont.fnt",
            "GJ_button_01.png",
            0.8f
        );

        auto button = CCMenuItemSpriteExtra::create(
            sprite,
            this,
            menu_selector(PrecisionEditorUI::onPrecision)
        );

        auto menu = CCMenu::create();

        menu->addChild(button);

        menu->setPosition(
            ccp(win.width - 65.f, win.height - 115.f)
        );

        this->addChild(menu, 9999);

        return true;
    }

    void onPrecision(CCObject*) {
        if (!m_selectedObject) {
            FLAlertLayer::create(
                "Extended Precision",
                "Select an object first.",
                "OK"
            )->show();

            return;
        }

        if (auto popup = PrecisionPopup::create(m_selectedObject)) {
            popup->show();
        }
    }
};
