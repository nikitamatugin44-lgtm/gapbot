
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>

#include <string>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

using namespace geode::prelude;

// ==========================================
// EXTENDED PRECISION
// ==========================================

// Decimal arbitrary precision storage.
// Values are stored as strings to avoid float rounding.

struct Decimal {
    bool negative = false;
    std::string digits = "0";
    int64_t scale = 0;
};

struct ExactPosition {
    Decimal x;
    Decimal y;
};

struct PrecisionConfig {
    bool enabled = true;
    std::string movementStep =
        "0.000000000000000000000001";
};

static PrecisionConfig g_config;

static std::unordered_map<
    GameObject*,
    ExactPosition
> g_positions;

// ==========================================
// DECIMAL HELPERS
// ==========================================

static std::string normalize(std::string value) {
    size_t first = value.find_first_not_of('0');

    if (first == std::string::npos)
        return "0";

    return value.substr(first);
}

static Decimal parseDecimal(const std::string& input) {
    Decimal result;

    if (input.empty())
        return result;

    size_t pos = 0;

    if (input[pos] == '-') {
        result.negative = true;
        pos++;
    }
    else if (input[pos] == '+') {
        pos++;
    }

    bool fractional = false;

    for (; pos < input.size(); pos++) {
        char c = input[pos];

        if (c == '.') {
            if (fractional)
                return Decimal{};

            fractional = true;
            continue;
        }

        if (c < '0' || c > '9')
            return Decimal{};

        result.digits += c;

        if (fractional)
            result.scale++;
    }

    // Remove leading zeroes.
    result.digits = normalize(result.digits);

    // Remove unnecessary trailing decimal zeroes.
    while (
        result.scale > 0 &&
        result.digits.size() > 1 &&
        result.digits.back() == '0'
    ) {
        result.digits.pop_back();
        result.scale--;
    }

    if (result.digits == "0")
        result.negative = false;

    return result;
}

static std::string decimalToString(const Decimal& value) {
    std::string digits = value.digits;

    if (value.scale > 0) {
        if (digits.size() <= value.scale) {
            digits.insert(
                0,
                value.scale - digits.size() + 1,
                '0'
            );
        }

        digits.insert(
            digits.size() - value.scale,
            "."
        );
    }

    if (value.negative && digits != "0")
        digits.insert(0, "-");

    return digits;
}

// ==========================================
// DECIMAL ADDITION
// ==========================================

static std::string addUnsigned(
    std::string a,
    std::string b
) {
    std::reverse(a.begin(), a.end());
    std::reverse(b.begin(), b.end());

    std::string result;

    int carry = 0;
    size_t length = std::max(a.size(), b.size());

    for (size_t i = 0; i < length; i++) {
        int x = i < a.size() ? a[i] - '0' : 0;
        int y = i < b.size() ? b[i] - '0' : 0;

        int sum = x + y + carry;

        result += char('0' + sum % 10);
        carry = sum / 10;
    }

    if (carry)
        result += char('0' + carry);

    std::reverse(result.begin(), result.end());

    return normalize(result);
}

static int compareUnsigned(
    const std::string& a,
    const std::string& b
) {
    if (a.size() != b.size())
        return a.size() > b.size() ? 1 : -1;

    if (a == b)
        return 0;

    return a > b ? 1 : -1;
}

static std::string subtractUnsigned(
    std::string a,
    std::string b
) {
    std::reverse(a.begin(), a.end());
    std::reverse(b.begin(), b.end());

    std::string result;

    int borrow = 0;

    for (size_t i = 0; i < a.size(); i++) {
        int x = a[i] - '0' - borrow;
        int y = i < b.size() ? b[i] - '0' : 0;

        borrow = 0;

        if (x < y) {
            x += 10;
            borrow = 1;
        }

        result += char('0' + x - y);
    }

    std::reverse(result.begin(), result.end());

    return normalize(result);
}

static Decimal addDecimal(
    Decimal a,
    Decimal b
) {
    int64_t scale = std::max(a.scale, b.scale);

    a.digits.append(scale - a.scale, '0');
    b.digits.append(scale - b.scale, '0');

    Decimal result;
    result.scale = scale;

    if (a.negative == b.negative) {
        result.digits = addUnsigned(
            a.digits,
            b.digits
        );

        result.negative = a.negative;
    }
    else {
        int cmp = compareUnsigned(
            a.digits,
            b.digits
        );

        if (cmp == 0)
            return Decimal{};

        if (cmp > 0) {
            result.digits = subtractUnsigned(
                a.digits,
                b.digits
            );

            result.negative = a.negative;
        }
        else {
            result.digits = subtractUnsigned(
                b.digits,
                a.digits
            );

            result.negative = b.negative;
        }
    }

    result.digits = normalize(result.digits);

    return result;
}

// ==========================================
// EXACT POSITION STORAGE
// ==========================================

static ExactPosition& getPosition(GameObject* object) {
    return g_positions[object];
}

static void applyExactStep(
    GameObject* object,
    bool horizontal,
    bool positive
) {
    if (!object)
        return;

    auto& position = getPosition(object);

    std::string step = g_config.movementStep;

    Decimal delta = parseDecimal(step);

    if (!positive)
        delta.negative = !delta.negative;

    if (horizontal)
        position.x = addDecimal(position.x, delta);
    else
        position.y = addDecimal(position.y, delta);
}

// ==========================================
// SETTINGS
// ==========================================

class PrecisionPopup : public Popup {
protected:
    TextInput* m_step = nullptr;

    void onSave(CCObject*) {
        std::string value = m_step->getString();

        Decimal parsed = parseDecimal(value);

        g_config.movementStep =
            decimalToString(parsed);

        Mod::get()->setSavedValue<std::string>(
            "movement-step",
            g_config.movementStep
        );
    }

    bool init() {
        if (!Popup::init(320.f, 180.f))
            return false;

        setTitle("Extended Precision");

        auto label = CCLabelBMFont::create(
            "Movement Step",
            "bigFont.fnt"
        );

        label->setScale(0.5f);

        m_mainLayer->addChildAtPosition(
            label,
            Anchor::Center,
            ccp(0.f, 55.f)
        );

        m_step = TextInput::create(
            270.f,
            "Movement Step",
            "bigFont.fnt"
        );

        m_step->setFilter("0123456789.-");
        m_step->setString(g_config.movementStep);

        m_mainLayer->addChildAtPosition(
            m_step,
            Anchor::Center,
            ccp(0.f, 15.f)
        );

        auto save = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("SAVE"),
            this,
            menu_selector(PrecisionPopup::onSave)
        );

        m_buttonMenu->addChildAtPosition(
            save,
            Anchor::Center,
            ccp(0.f, -55.f)
        );

        return true;
    }

public:
    static PrecisionPopup* create() {
        auto ret = new PrecisionPopup();

        if (ret->init()) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};

// ==========================================
// EDITOR HOOK
// ==========================================

class $modify(PrecisionEditorUI, EditorUI) {
    bool init(LevelEditorLayer* layer) {
        if (!EditorUI::init(layer))
            return false;

        g_config.enabled =
            Mod::get()->getSettingValue<bool>("enabled");

        g_config.movementStep =
            Mod::get()->getSavedValue<std::string>(
                "movement-step",
                "0.000000000000000000000001"
            );

        return true;
    }

    void moveObjectCall(EditCommand command) {
        EditorUI::moveObjectCall(command);
    }
};
