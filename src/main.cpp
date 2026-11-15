
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>

#include <string>
#include <cstdlib>
#include <cmath>

using namespace geode::prelude;

static bool g_enabled = true;
static std::string g_step = "0.000001";

class PrecisionPopup : public Popup {
    TextInput* m_input = nullptr;

    void onSave(CCObject*) {
        auto value = m_input->getString();

        std::string s = value;
        if (s.empty()) return;

        char* end = nullptr;
        float parsed = std::strtof(s.c_str(), &end);

        if (end == s.c_str() || *end != '\0' ||
            !std::isfinite(parsed) || parsed <= 0.f) {
            FLAlertLayer::create(
                "Extended Precision",
                "Invalid movement step.",
                "OK"
            )->show();
            return;
        }

        g_step = s;
        Mod::get()->setSavedValue<std::string>(
            "movement-step", g_step
        );
        onClose(nullptr);
    }

    bool setup() {
        if (!Popup::init(300.f, 160.f))
            return false;

        setTitle("Extended Precision");

        auto label = CCLabelBMFont::create(
            "Movement Step", "bigFont.fnt"
        );
        label->setScale(0.55f);
        m_mainLayer->addChildAtPosition(
            label, Anchor::Center, ccp(0, 38)
        );

        m_input = TextInput::create(
            250.f, "Enter step", "bigFont.fnt"
        );
        m_input->setFilter("0123456789.-");
        m_input->setString(g_step);

        m_mainLayer->addChildAtPosition(
            m_input, Anchor::Center, ccp(0, 0)
        );

        auto save = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("SAVE"),
            this,
            menu_selector(PrecisionPopup::onSave)
        );

        m_buttonMenu->addChildAtPosition(
            save, Anchor::Center, ccp(0, -48)
        );

        return true;
    }

public:
    static PrecisionPopup* create() {
        auto ret = new PrecisionPopup();
        if (ret && ret->setup()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

class $modify(PrecisionEditorUI, EditorUI) {
    bool init(LevelEditorLayer* layer) {
        if (!EditorUI::init(layer))
            return false;

        g_enabled =
            Mod::get()->getSettingValue<bool>("enabled");

        g_step = Mod::get()->getSavedValue<std::string>(
            "movement-step",
            "0.000001"
        );

        return true;
    }

    void setupEditMenu() {
        EditorUI::setupEditMenu();

        if (!m_editButtonBar)
            return;

        if (m_editButtonBar->getChildByTag(987654))
            return;

        auto menu = CCMenu::create();
        menu->setPosition(CCPointZero);
        menu->setTag(987654);

        auto button = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("EP", 30, true, "bigFont.fnt", "GJ_button_04.png", 30.f, 0.7f),
            this,
            menu_selector(PrecisionEditorUI::onPrecision)
        );

        menu->addChild(button);

        auto size = m_editButtonBar->getContentSize();
        button->setPosition(
            ccp(size.width - 25.f, size.height - 25.f)
        );

        m_editButtonBar->addChild(menu, 999);
    }

    void onPrecision(CCObject*) {
        auto popup = PrecisionPopup::create();
        if (popup)
            popup->show();
    }

    void moveObjectCall(EditCommand command) {
        if (!g_enabled) {
            EditorUI::moveObjectCall(command);
            return;
        }

        char* end = nullptr;
        float step = std::strtof(g_step.c_str(), &end);

        if (end == g_step.c_str() || *end != '\0' ||
            !std::isfinite(step) || step <= 0.f) {
            EditorUI::moveObjectCall(command);
            return;
        }

        auto originalOffset = this->moveForCommand(command);

        if (originalOffset == CCPointZero) {
            EditorUI::moveObjectCall(command);
            return;
        }

        CCPoint offset(
            originalOffset.x == 0.f
                ? 0.f
                : std::copysign(step, originalOffset.x),
            originalOffset.y == 0.f
                ? 0.f
                : std::copysign(step, originalOffset.y)
        );

        if (!m_selectedObjects || m_selectedObjects->count() == 0) {
            EditorUI::moveObjectCall(command);
            return;
        }

        CCARRAY_FOREACH(m_selectedObjects, node) {
            auto object = static_cast<GameObject*>(node);
            if (object)
                this->moveObject(object, offset);
        }
    }
};
