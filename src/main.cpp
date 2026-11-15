
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

#include <sstream>
#include <iomanip>
#include <string>

using namespace geode::prelude;

class $modify(PrecisionEditorUI, EditorUI) {
    CCLabelBMFont* m_precisionLabel = nullptr;

    bool init(LevelEditorLayer* layer) {
        if (!EditorUI::init(layer))
            return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        m_precisionLabel = CCLabelBMFont::create(
            "Position: (0, 0)",
            "bigFont.fnt"
        );

        if (m_precisionLabel) {
            m_precisionLabel->setAnchorPoint({0.f, 1.f});
            m_precisionLabel->setPosition({
                12.f,
                winSize.height - 125.f
            });
            m_precisionLabel->setScale(0.42f);
            m_precisionLabel->setColor({255, 255, 255});
            m_precisionLabel->setZOrder(9999);

            this->addChild(m_precisionLabel);
        }

        this->schedule(
            schedule_selector(
                PrecisionEditorUI::updatePrecisionInfo
            ),
            0.05f
        );

        return true;
    }

    void updatePrecisionInfo(float dt) {
        if (!m_precisionLabel)
            return;

        if (!m_selectedObject) {
            m_precisionLabel->setString("Position: (no object)");
            return;
        }

        auto pos = m_selectedObject->getPosition();

        std::ostringstream stream;

        stream << std::fixed << std::setprecision(17)
               << "Position: ("
               << pos.x << ", "
               << pos.y << ")";

        m_precisionLabel->setString(stream.str().c_str());
    }
};
