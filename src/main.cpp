
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

#include <string>

using namespace geode::prelude;

class $modify(FakePositionEditor, EditorUI) {

    void updateObjectInfoLabel() {
        // Сначала обновляем оригинальную информацию GD
        EditorUI::updateObjectInfoLabel();

        if (!m_objectInfoLabel || !m_selectedObject)
            return;

        bool enabled = Mod::get()->getSettingValue<bool>(
            "fake-enabled"
        );

        if (!enabled)
            return;

        std::string fakeX =
            Mod::get()->getSettingValue<std::string>("fake-x");

        std::string fakeY =
            Mod::get()->getSettingValue<std::string>("fake-y");

        std::string info = m_objectInfoLabel->getString();

        const std::string marker = "Position: (";
        auto start = info.find(marker);

        if (start == std::string::npos)
            return;

        start += marker.length();

        auto end = info.find(')', start);

        if (end == std::string::npos)
            return;

        std::string fakePosition = fakeX + ", " + fakeY;

        info.replace(
            start,
            end - start,
            fakePosition
        );

        m_objectInfoLabel->setString(info.c_str());
    }
};
