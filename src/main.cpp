
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>

using namespace geode::prelude;

class $modify(ExtendedPrecision, EditorUI) {

    void updateObjectInfoLabel() {
        // Обновляем стандартную информацию GD
        EditorUI::updateObjectInfoLabel();

        if (!m_objectInfoLabel || !m_selectedObject)
            return;

        // Читаем количество знаков после запятой
        int digits = Mod::get()->getSettingValue<int>(
            "decimal-places"
        );

        digits = std::clamp(digits, 0, 100);

        // Получаем реальные координаты объекта
        auto pos = m_selectedObject->getPosition();

        std::ostringstream coords;

        coords << std::fixed
               << std::setprecision(digits)
               << pos.x << ", "
               << pos.y;

        // Получаем стандартный текст информации
        std::string info = m_objectInfoLabel->getString();

        // Ищем строку Position
        const std::string marker = "Position: (";
        auto start = info.find(marker);

        if (start == std::string::npos)
            return;

        start += marker.length();

        auto end = info.find(')', start);

        if (end == std::string::npos)
            return;

        // Заменяем только координаты
        info.replace(
            start,
            end - start,
            coords.str()
        );

        m_objectInfoLabel->setString(info.c_str());
    }
};
