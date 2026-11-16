
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

class ExtendedPrecisionPopup : public geode::Popup {
protected:
    CCMenu* m_headerMenu = nullptr;
    CCNode* m_window = nullptr;
    CCLabelBMFont* m_positionLabel = nullptr;

    bool init() {
        if (!Popup::init(300.f, 230.f))
            return false;

        this->setTitle("EXTENDED PRECISION");

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        m_window = this;

        auto panel = CCScale9Sprite::create(
            "square02_001.png"
        );

        panel->setContentSize({280.f, 205.f});
        panel->setColor({25, 30, 43});
        panel->setPosition({150.f, 105.f});
        this->addChild(panel, -1);

        auto header = CCScale9Sprite::create(
            "square02_001.png"
        );

        header->setContentSize({280.f, 34.f});
        header->setColor({45, 65, 100});
        header->setPosition({150.f, 190.f});
        this->addChild(header);

        auto headerLabel = CCLabelBMFont::create(
            "EXTENDED PRECISION",
            "bigFont.fnt"
        );

        headerLabel->setScale(0.42f);
        headerLabel->setPosition({140.f, 190.f});
        this->addChild(headerLabel, 2);

        auto close = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("X"),
            this,
            menu_selector(ExtendedPrecisionPopup::onCloseWindow)
        );

        auto closeMenu = CCMenu::create();
        closeMenu->addChild(close);
        closeMenu->setPosition({275.f, 190.f});
        this->addChild(closeMenu, 5);

        auto diagnostics = CCLabelBMFont::create(
            "DIAGNOSTICS: READY",
            "bigFont.fnt"
        );

        diagnostics->setScale(0.45f);
        diagnostics->setPosition({150.f, 155.f});
        this->addChild(diagnostics);

        m_positionLabel = CCLabelBMFont::create(
            "X: ---\nY: ---",
            "bigFont.fnt"
        );

        m_positionLabel->setScale(0.55f);
        m_positionLabel->setAlignment(kCCTextAlignmentLeft);
        m_positionLabel->setPosition({75.f, 115.f});
        this->addChild(m_positionLabel);

        auto info = CCLabelBMFont::create(
            "DOUBLE PRECISION ENGINE",
            "bigFont.fnt"
        );

        info->setScale(0.4f);
        info->setPosition({150.f, 65.f});
        this->addChild(info);

        auto status = CCLabelBMFont::create(
            "ACCUMULATOR: DISABLED",
            "bigFont.fnt"
        );

        status->setScale(0.4f);
        status->setColor({120, 220, 140});
        status->setPosition({150.f, 40.f});
        this->addChild(status);

        return true;
    }

    void onCloseWindow(CCObject*) {
        this->onClose(nullptr);
    }

public:
    static ExtendedPrecisionPopup* create() {
        auto ret = new ExtendedPrecisionPopup();

        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};

class $modify(PrecisionEditorUI, EditorUI) {
    bool init(LevelEditorLayer* layer) {
        if (!EditorUI::init(layer))
            return false;

        auto sprite = ButtonSprite::create("EP");

        auto button = CCMenuItemSpriteExtra::create(
            sprite,
            this,
            menu_selector(PrecisionEditorUI::onOpenEP)
        );

        auto menu = CCMenu::create();
        menu->addChild(button);
        menu->setPosition({40.f, 120.f});

        this->addChild(menu, 1000);

        return true;
    }

    void onOpenEP(CCObject*) {
        ExtendedPrecisionPopup::create()->show();
    }
};
