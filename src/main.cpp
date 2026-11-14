#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PauseLayer.hpp>

#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

using namespace geode::prelude;

// ==========================================
// CONFIGURATION
// ==========================================

struct Zone {
    float x0 = 0.f;
    float x1 = 0.f;
    float y = 0.f;
};

struct Config {
    bool enabled = true;
    bool always = false;

    int hold = 1;
    int release = 1;

    float approach = 80.f;

    std::vector<Zone> zones;
};

static Config g_cfg;
static bool g_reload = true;

// ==========================================
// STRING HELPERS
// ==========================================

static std::vector<std::string> splitStr(
    const std::string& s,
    char delimiter
) {
    std::vector<std::string> result;

    size_t start = 0;

    while (true) {
        size_t pos = s.find(delimiter, start);

        if (pos == std::string::npos) {
            result.push_back(s.substr(start));
            break;
        }

        result.push_back(s.substr(start, pos - start));
        start = pos + 1;
    }

    return result;
}

static std::string val(TextInput* input) {
    if (!input) return "";
    return input->getString();
}

// ==========================================
// CONFIG LOADING
// ==========================================

static void loadConfig() {
    auto mod = Mod::get();

    g_cfg.enabled = mod->getSavedValue<bool>(
        "gs-enabled", true
    );

    g_cfg.always = mod->getSavedValue<bool>(
        "gs-always", false
    );

    std::string hold = mod->getSavedValue<std::string>(
        "gs-hold", "1"
    );

    std::string release = mod->getSavedValue<std::string>(
        "gs-release", "1"
    );

    std::string approach = mod->getSavedValue<std::string>(
        "gs-approach", "80"
    );

    g_cfg.hold = std::max(1, std::atoi(hold.c_str()));
    g_cfg.release = std::max(1, std::atoi(release.c_str()));

    g_cfg.approach = std::max(
        0.f,
        std::strtof(approach.c_str(), nullptr)
    );

    g_cfg.zones.clear();

    std::string zones = mod->getSavedValue<std::string>(
        "gs-zones", ""
    );

    for (auto& row : splitStr(zones, ',')) {
        auto fields = splitStr(row, ':');

        if (fields.size() < 3)
            continue;

        if (fields[0].empty() ||
            fields[1].empty() ||
            fields[2].empty())
            continue;

        Zone z;

        z.x0 = std::strtof(fields[0].c_str(), nullptr);
        z.x1 = std::strtof(fields[1].c_str(), nullptr);
        z.y = std::strtof(fields[2].c_str(), nullptr);

        if (z.x0 > z.x1)
            std::swap(z.x0, z.x1);

        g_cfg.zones.push_back(z);
    }

    g_reload = false;
}

// ==========================================
// POPUP
// ==========================================

class SpamPopup : public Popup {
protected:

    TextInput* m_hold = nullptr;
    TextInput* m_release = nullptr;
    TextInput* m_approach = nullptr;

    std::vector<std::vector<TextInput*>> m_cells;

    // --------------------------------------
    // INPUT CREATION
    // --------------------------------------

    TextInput* makeInput(
        float width,
        const char* hint,
        float x,
        float y
    ) {
        auto input = TextInput::create(
            width,
            hint,
            "bigFont.fnt"
        );

        input->setFilter("0123456789.-");
        input->setScale(0.6f);

        m_mainLayer->addChildAtPosition(
            input,
            Anchor::Center,
            ccp(x, y)
        );

        return input;
    }

    // --------------------------------------
    // LABEL CREATION
    // --------------------------------------

    CCLabelBMFont* makeLabel(
        const char* text,
        const char* font,
        float scale,
        float x,
        float y,
        bool left = false
    ) {
        auto label = CCLabelBMFont::create(text, font);

        label->setScale(scale);

        if (left)
            label->setAnchorPoint({0.f, 0.5f});

        m_mainLayer->addChildAtPosition(
            label,
            Anchor::Center,
            ccp(x, y)
        );

        return label;
    }

    // --------------------------------------
    // SAVE ALL SETTINGS
    // --------------------------------------

    void saveFromInputs() {
        auto mod = Mod::get();

        if (m_hold) {
            mod->setSavedValue<std::string>(
                "gs-hold", val(m_hold)
            );
        }

        if (m_release) {
            mod->setSavedValue<std::string>(
                "gs-release", val(m_release)
            );
        }

        if (m_approach) {
            mod->setSavedValue<std::string>(
                "gs-approach", val(m_approach)
            );
        }

        std::string output;

        for (auto& row : m_cells) {
            if (row.size() < 3)
                continue;

            std::string x0 = val(row[0]);
            std::string x1 = val(row[1]);
            std::string y = val(row[2]);

            if (x0.empty() || x1.empty() || y.empty())
                continue;

            if (!output.empty())
                output += ",";

            output += x0 + ":" + x1 + ":" + y;
        }

        mod->setSavedValue<std::string>(
            "gs-zones", output
        );

        // Immediately reload configuration
        loadConfig();
    }

    // --------------------------------------
    // SAVE BUTTON
    // --------------------------------------

    void onSave(CCObject*) {
        saveFromInputs();
    }

    // --------------------------------------
    // ENABLE SWITCH
    // --------------------------------------

    void onEnable(CCObject* sender) {
        bool enabled =
            !static_cast<CCMenuItemToggler*>(sender)->isToggled();

        Mod::get()->setSavedValue<bool>(
            "gs-enabled", enabled
        );

        g_reload = true;
    }

    // --------------------------------------
    // ALWAYS SWITCH
    // --------------------------------------

    void onAlways(CCObject* sender) {
        bool enabled =
            !static_cast<CCMenuItemToggler*>(sender)->isToggled();

        Mod::get()->setSavedValue<bool>(
            "gs-always", enabled
        );

        g_reload = true;
    }

    // --------------------------------------
    // INITIALIZATION
    // --------------------------------------

    bool init() {
        if (!Popup::init(400.f, 310.f))
            return false;

        this->setTitle("Gap Spam");

        if (g_reload)
            loadConfig();

        auto mod = Mod::get();

        // ==================================
        // SWITCHES
        // ==================================

        auto enableToggle =
            CCMenuItemToggler::createWithStandardSprites(
                this,
                menu_selector(SpamPopup::onEnable),
                0.6f
            );

        enableToggle->toggle(g_cfg.enabled);

        m_buttonMenu->addChildAtPosition(
            enableToggle,
            Anchor::Center,
            ccp(-175.f, 120.f)
        );

        makeLabel(
            "Enable",
            "bigFont.fnt",
            0.4f,
            -158.f,
            120.f,
            true
        );

        auto alwaysToggle =
            CCMenuItemToggler::createWithStandardSprites(
                this,
                menu_selector(SpamPopup::onAlways),
                0.6f
            );

        alwaysToggle->toggle(g_cfg.always);

        m_buttonMenu->addChildAtPosition(
            alwaysToggle,
            Anchor::Center,
            ccp(-60.f, 120.f)
        );

        makeLabel(
            "Always spam",
            "bigFont.fnt",
            0.4f,
            -43.f,
            120.f,
            true
        );

        // ==================================
        // SPAM SETTINGS
        // ==================================

        std::string holdSaved =
            mod->getSavedValue<std::string>("gs-hold", "1");

        std::string releaseSaved =
            mod->getSavedValue<std::string>("gs-release", "1");

        std::string approachSaved =
            mod->getSavedValue<std::string>("gs-approach", "80");

        makeLabel(
            "Hold",
            "bigFont.fnt",
            0.35f,
            -185.f,
            90.f,
            true
        );

        m_hold = makeInput(
            50.f, "1", -105.f, 90.f
        );

        m_hold->setString(holdSaved);

        makeLabel(
            "Release",
            "bigFont.fnt",
            0.35f,
            -70.f,
            90.f,
            true
        );

        m_release = makeInput(
            50.f, "1", 25.f, 90.f
        );

        m_release->setString(releaseSaved);

        makeLabel(
            "Approach",
            "bigFont.fnt",
            0.35f,
            70.f,
            90.f,
            true
        );

        m_approach = makeInput(
            60.f, "80", 155.f, 90.f
        );

        m_approach->setString(approachSaved);

        // ==================================
        // ZONES TABLE
        // ==================================

        const float xs[3] = {
            -100.f, 0.f, 100.f
        };

        const char* headers[3] = {
            "X from",
            "X to",
            "Y height"
        };

        for (int c = 0; c < 3; c++) {
            makeLabel(
                headers[c],
                "goldFont.fnt",
                0.4f,
                xs[c],
                62.f
            );
        }

        // Load saved zones into inputs

        std::vector<std::vector<std::string>> saved;

        std::string zonesSaved =
            mod->getSavedValue<std::string>(
                "gs-zones", ""
            );

        for (auto& row : splitStr(zonesSaved, ',')) {
            auto fields = splitStr(row, ':');

            if (fields.size() >= 3)
                saved.push_back(fields);
        }

        // Five editable rows

        for (int r = 0; r < 5; r++) {
            std::vector<TextInput*> row;

            float y = 38.f - 23.f * r;

            for (int c = 0; c < 3; c++) {
                auto input = makeInput(
                    90.f,
                    c == 2 ? "Y" : "X",
                    xs[c],
                    y
                );

                if (r < static_cast<int>(saved.size())) {
                    input->setString(saved[r][c]);
                }

                row.push_back(input);
            }

            m_cells.push_back(row);
        }

        // ==================================
        // PLAYER COORDINATES
        // ==================================

        std::string info = "PLAYER 1 | X: unavailable | Y: unavailable";

        auto playLayer = PlayLayer::get();

        if (playLayer && playLayer->m_player1) {
            auto player = playLayer->m_player1;

            info = fmt::format(
                "PLAYER 1 | X: {:.2f} | Y: {:.2f}",
                player->getPositionX(),
                player->getPositionY()
            );
        }

        makeLabel(
            info.c_str(),
            "goldFont.fnt",
            0.38f,
            0.f,
            -100.f
        );

        // ==================================
        // HELP TEXT
        // ==================================

        makeLabel(
            "Empty rows are ignored.",
            "bigFont.fnt",
            0.27f,
            0.f,
            -119.f
        );

        makeLabel(
            "30 units = 1 block | 1 / 1 = every physics step",
            "bigFont.fnt",
            0.24f,
            0.f,
            -134.f
        );

        // ==================================
        // SAVE BUTTON
        // ==================================

        auto saveButton = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("SAVE"),
            this,
            menu_selector(SpamPopup::onSave)
        );

        m_buttonMenu->addChildAtPosition(
            saveButton,
            Anchor::Center,
            ccp(155.f, -137.f)
        );

        return true;
    }

public:

    static SpamPopup* create() {
        auto ret = new SpamPopup();

        if (ret->init()) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};

// ==========================================
// PLAYER MODIFICATION
// ==========================================

class $modify(SpamPlayer, PlayerObject) {
public:
    struct Fields {
        float lastY = 0.f;
        float correctionY = 0.f;
        float bestY = 0.f;
        float bestError = 999999.f;

        bool haveLast = false;
        bool lastHold = false;
        bool phaseHold = false;
        bool correcting = false;
        bool correctionLocked = false;

        int phaseCount = 0;
        int activeZone = -1;
    };

    void setHold(bool hold) {
        auto f = m_fields.self();

        if (f->haveLast && f->lastHold == hold)
            return;

        if (hold)
            this->pushButton(static_cast<PlayerButton>(1));
        else
            this->releaseButton(static_cast<PlayerButton>(1));

        f->lastHold = hold;
        f->haveLast = true;
    }

    void spamStep() {
        auto f = m_fields.self();

        if (f->phaseCount <= 0) {
            f->phaseHold = !f->phaseHold;
            f->phaseCount = f->phaseHold
                ? g_cfg.hold
                : g_cfg.release;
        }

        f->phaseCount--;
        this->setHold(f->phaseHold);
    }

    void resetPositioning() {
        auto f = m_fields.self();

        f->correcting = false;
        f->correctionLocked = false;
        f->bestError = 999999.f;
        f->phaseCount = 0;
        f->activeZone = -1;
        f->haveLast = false;
    }

    void update(float dt) {
        PlayerObject::update(dt);

        if (g_reload)
            loadConfig();

        auto f = m_fields.self();

        float py = this->getPositionY();
        float px = this->getPositionX();

        if (!g_cfg.enabled) {
            resetPositioning();
            return;
        }

        if (g_cfg.always) {
            f->activeZone = -1;
            f->correcting = false;
            f->correctionLocked = false;
            spamStep();
            return;
        }

        if (!m_isDart || g_cfg.zones.empty()) {
            resetPositioning();
            return;
        }

        // Find active zone
        int zoneIndex = -1;

        for (int i = 0; i < (int)g_cfg.zones.size(); i++) {
            auto& z = g_cfg.zones[i];

            if (px >= z.x0 - g_cfg.approach && px <= z.x1) {
                zoneIndex = i;
                break;
            }
        }

        if (zoneIndex == -1) {
            resetPositioning();
            return;
        }

        auto& zone = g_cfg.zones[zoneIndex];

        // New zone
        if (f->activeZone != zoneIndex) {
            f->activeZone = zoneIndex;
            f->correcting = true;
            f->correctionLocked = false;
            f->correctionY = zone.y;
            f->bestY = py;
            f->bestError = std::fabs(zone.y - py);
            f->lastY = py;
            f->phaseCount = 0;
        }

        if (!f->correcting) {
            spamStep();
            return;
        }

        // Initialize previous position
        if (!f->haveLast) {
            f->lastY = py;
            f->haveLast = true;
        }

        float previousY = f->lastY;
        float targetY = f->correctionY;

        float previousError = targetY - previousY;
        float currentError = targetY - py;

        float currentDistance = std::fabs(zone.y - py);

        // Remember the closest position to the requested Y
        if (currentDistance < f->bestError) {
            f->bestError = currentDistance;
            f->bestY = py;
        }

        // Detect crossing the target height
        bool crossed =
            (previousError > 0.f && currentError < 0.f) ||
            (previousError < 0.f && currentError > 0.f);

        if (crossed && !f->correctionLocked) {
            // Choose the closest of the two observed positions
            float previousDistance = std::fabs(zone.y - previousY);
            float currentDistance = std::fabs(zone.y - py);

            f->correctionY =
                previousDistance <= currentDistance
                ? previousY
                : py;

            f->correctionLocked = true;
        }

        // After crossing, return to the closest observed position
        if (f->correctionLocked) {
            float correctionError = f->correctionY - py;

            if (std::fabs(correctionError) <= 0.00001f) {
                f->lastY = py;
                f->correcting = false;
                f->phaseCount = 0;

                spamStep();
                return;
            }

            bool wantUp = correctionError > 0.f;
            bool hold = (wantUp == !m_isUpsideDown);

            f->lastY = py;
            setHold(hold);
            return;
        }

        // Exact target reached
        if (std::fabs(currentError) <= 0.00001f) {
            f->correctionY = py;
            f->lastY = py;
            f->correcting = false;
            spamStep();
            return;
        }

        // Continue moving towards target
        bool wantUp = currentError > 0.f;
        bool hold = (wantUp == !m_isUpsideDown);

        f->lastY = py;
        setHold(hold);
    }
};

    // --------------------------------------
    // BUTTON CONTROL
    // --------------------------------------

    void setHold(bool hold) {
        auto f = m_fields.self();

        if (f->haveLast && f->lastHold == hold)
            return;

        if (hold) {
            this->pushButton(
                static_cast<PlayerButton>(1)
            );
        }
        else {
            this->releaseButton(
                static_cast<PlayerButton>(1)
            );
        }

        f->lastHold = hold;
        f->haveLast = true;
    }

    // --------------------------------------
    // SPAM PATTERN
    // --------------------------------------

    void spamStep() {
        auto f = m_fields.self();

        if (f->phaseCount <= 0) {
            f->phaseHold = !f->phaseHold;

            f->phaseCount = f->phaseHold
                ? g_cfg.hold
                : g_cfg.release;
        }

        f->phaseCount--;

        this->setHold(f->phaseHold);
    }

    // --------------------------------------
    // UPDATE
    // --------------------------------------

    void update(float dt) {
        PlayerObject::update(dt);

        if (g_reload)
            loadConfig();

        auto f = m_fields.self();

        float py = this->getPositionY();

        float stepY = std::fabs(
            py - f->lastY
        );

        f->lastY = py;

        // Disabled

        if (!g_cfg.enabled) {
            f->haveLast = false;
            f->phaseCount = 0;
            return;
        }

        // Autoclicker

        if (g_cfg.always) {
            this->spamStep();
            return;
        }

        // Only wave

        if (!m_isDart || g_cfg.zones.empty()) {
            f->haveLast = false;
            f->phaseCount = 0;
            return;
        }

        float px = this->getPositionX();

        const Zone* zone = nullptr;

        for (auto& z : g_cfg.zones) {
            if (
                px >= z.x0 - g_cfg.approach &&
                px <= z.x1
            ) {
                zone = &z;
                break;
            }
        }

        if (!zone) {
            f->haveLast = false;
            f->phaseCount = 0;
            return;
        }

        float error = zone->y - py;

        float tolerance = std::max(
            0.05f,
            stepY * 2.5f
        );

        // Reached target height

        if (std::fabs(error) <= tolerance) {
            this->spamStep();
            return;
        }

        // Fly towards target height

        f->phaseCount = 0;

        bool wantUp = error > 0.f;

        bool hold = (
            wantUp == !m_isUpsideDown
        );

        this->setHold(hold);
    }
};

// ==========================================
// PAUSE MENU
// ==========================================

class $modify(SpamPause, PauseLayer) {

    void customSetup() {
        PauseLayer::customSetup();

        auto win = CCDirector::get()->getWinSize();

        // Independent button

        auto icon = ButtonSprite::create("Spam");

        icon->setScale(0.75f);

        auto button = CCMenuItemSpriteExtra::create(
            icon,
            this,
            menu_selector(SpamPause::onSpam)
        );

        button->setID("gap-spam-button"_spr);

        // Separate menu:
        // Does not modify original pause buttons.

        auto menu = CCMenu::create();

        menu->setID("gap-spam-menu"_spr);

        menu->addChild(button);

        button->setPosition({0.f, 0.f});

        menu->setPosition({
            win.width - 45.f,
            win.height - 75.f
        });

        this->addChild(menu, 100);
    }

    void onSpam(CCObject*) {
        if (auto popup = SpamPopup::create()) {
            popup->show();
        }
    }
};
