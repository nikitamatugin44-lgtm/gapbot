#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>

using namespace geode::prelude;

static void replaceAll(
    std::string& text,
    const std::string& from,
    const std::string& to
) {
    if (from.empty())
        return;

    size_t pos = 0;

    while ((pos = text.find(from, pos)) != std::string::npos) {
        text.replace(pos, from.length(), to);
        pos += to.length();
    }
}

static std::string formatNumber(double value, int digits) {
    std::ostringstream stream;

    stream << std::fixed
           << std::setprecision(digits)
           << value;

    return stream.str();
}

class $modify(ExtendedPrecision, EditorUI) {

    void updateObjectInfoLabel() {
        EditorUI::updateObjectInfoLabel();

        if (!m_objectInfoLabel || !m_selectedObject)
            return;

        int digits = Mod::get()->getSettingValue<int>(
            "decimal-places"
        );

        digits = std::clamp(digits, 0, 100);

        std::string format =
            Mod::get()->getSettingValue<std::string>(
                "position-format"
            );

        auto pos = m_selectedObject->getPosition();

        std::string x = formatNumber(pos.x, digits);
        std::string y = formatNumber(pos.y, digits);

        replaceAll(format, "{x}", x);
        replaceAll(format, "{y}", y);

        std::string info = m_objectInfoLabel->getString();

        const std::string marker = "Position: (";

        auto start = info.find(marker);

        if (start == std::string::npos)
            return;

        auto end = info.find(')', start);

        if (end == std::string::npos)
            return;

        info.replace(
            start,
            end - start + 1,
            format
        );

        m_objectInfoLabel->setString(info.c_str());
    }
};
