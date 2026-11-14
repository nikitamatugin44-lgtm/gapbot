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

// a place where the wave must spam
struct Zone {
	float x0;   // zone starts here
	float x1;   // zone ends here
	float y;    // the height the wave flies to and spams at
};

struct Config {
	bool enabled = true;
	bool always = false;     // autoclicker: spam everywhere, in every gamemode
	int hold = 1;            // steps pressed
	int release = 1;         // steps released
	float approach = 80.f;   // start flying to y this many units before the zone
	std::vector<Zone> zones;
};

static Config g_cfg;
static bool g_reload = true;

static std::vector<std::string> splitStr(const std::string& s, char d) {
	std::vector<std::string> out;
	size_t i = 0;
	while (true) {
		size_t j = s.find(d, i);
		if (j == std::string::npos) {
			out.push_back(s.substr(i));
			break;
		}
		out.push_back(s.substr(i, j - i));
		i = j + 1;
	}
	return out;
}

// text of an input field as a normal std::string
static std::string val(TextInput* in) {
	std::string s = in->getString();
	return s;
}

// zones are saved as text: "x0:x1:y,x0:x1:y"
static void loadConfig() {
	auto mod = Mod::get();

	g_cfg.enabled = mod->getSavedValue<bool>("gs-enabled", true);
	g_cfg.always = mod->getSavedValue<bool>("gs-always", false);

	std::string hold = mod->getSavedValue<std::string>("gs-hold", "1");
	std::string release = mod->getSavedValue<std::string>("gs-release", "1");
	std::string approach = mod->getSavedValue<std::string>("gs-approach", "80");

	g_cfg.hold = std::max(1, std::atoi(hold.c_str()));
	g_cfg.release = std::max(1, std::atoi(release.c_str()));
	g_cfg.approach = std::max(0.f, std::strtof(approach.c_str(), nullptr));

	g_cfg.zones.clear();
	std::string zones = mod->getSavedValue<std::string>("gs-zones", "");
	for (auto& row : splitStr(zones, ',')) {
		auto f = splitStr(row, ':');
		if (f.size() < 3) continue;
		if (f[0].empty() || f[1].empty() || f[2].empty()) continue;

		Zone z;
		z.x0 = std::strtof(f[0].c_str(), nullptr);
		z.x1 = std::strtof(f[1].c_str(), nullptr);
		z.y = std::strtof(f[2].c_str(), nullptr);
		g_cfg.zones.push_back(z);
	}

	g_reload = false;
}

// the settings window
class SpamPopup : public Popup {
protected:
	TextInput* m_hold = nullptr;
	TextInput* m_release = nullptr;
	TextInput* m_approach = nullptr;
	std::vector<std::vector<TextInput*>> m_cells;   // [row][x0, x1, y]

	TextInput* makeInput(float width, const char* hint, float x, float y) {
		auto in = TextInput::create(width, hint, "bigFont.fnt");
		in->setFilter("0123456789.-");
		in->setScale(0.6f);
		m_mainLayer->addChildAtPosition(in, Anchor::Center, ccp(x, y));
		return in;
	}

	CCLabelBMFont* makeLabel(const char* text, const char* font, float scale, float x, float y, bool left) {
		auto l = CCLabelBMFont::create(text, font);
		l->setScale(scale);
		if (left) l->setAnchorPoint({ 0.f, 0.5f });
		m_mainLayer->addChildAtPosition(l, Anchor::Center, ccp(x, y));
		return l;
	}

	bool init() {
		if (!Popup::init(400.f, 270.f)) return false;

		this->setTitle("Gap Spam");

		if (g_reload) loadConfig();
		auto mod = Mod::get();

		// ---- switches ----
		auto en = CCMenuItemToggler::createWithStandardSprites(
			this, menu_selector(SpamPopup::onEnable), 0.6f
		);
		en->toggle(g_cfg.enabled);
		m_buttonMenu->addChildAtPosition(en, Anchor::Center, ccp(-175.f, 92.f));
		this->makeLabel("Enable", "bigFont.fnt", 0.4f, -158.f, 92.f, true);

		auto al = CCMenuItemToggler::createWithStandardSprites(
			this, menu_selector(SpamPopup::onAlways), 0.6f
		);
		al->toggle(g_cfg.always);
		m_buttonMenu->addChildAtPosition(al, Anchor::Center, ccp(-60.f, 92.f));
		this->makeLabel("Always spam (autoclicker)", "bigFont.fnt", 0.4f, -43.f, 92.f, true);

		// ---- spam pattern ----
		std::string holdSaved = mod->getSavedValue<std::string>("gs-hold", "1");
		std::string releaseSaved = mod->getSavedValue<std::string>("gs-release", "1");
		std::string approachSaved = mod->getSavedValue<std::string>("gs-approach", "80");

		this->makeLabel("Hold steps", "bigFont.fnt", 0.35f, -185.f, 66.f, true);
		m_hold = this->makeInput(50.f, "1", -105.f, 66.f);
		m_hold->setString(holdSaved);

		this->makeLabel("Release steps", "bigFont.fnt", 0.35f, -70.f, 66.f, true);
		m_release = this->makeInput(50.f, "1", 25.f, 66.f);
		m_release->setString(releaseSaved);

		this->makeLabel("Approach", "bigFont.fnt", 0.35f, 70.f, 66.f, true);
		m_approach = this->makeInput(60.f, "80", 155.f, 66.f);
		m_approach->setString(approachSaved);

		// ---- zones table ----
		const float xs[3] = { -100.f, 0.f, 100.f };
		const char* heads[3] = { "X from", "X to", "Y (height)" };
		for (int c = 0; c < 3; c++) {
			this->makeLabel(heads[c], "goldFont.fnt", 0.4f, xs[c], 40.f, false);
		}

		// saved zones are shown in the first rows
		std::vector<std::vector<std::string>> saved;
		std::string zonesSaved = mod->getSavedValue<std::string>("gs-zones", "");
		for (auto& row : splitStr(zonesSaved, ',')) {
			auto f = splitStr(row, ':');
			if (f.size() >= 3) saved.push_back(f);
		}

		for (int r = 0; r < 5; r++) {
			std::vector<TextInput*> row;
			float y = 16.f - 21.f * static_cast<float>(r);

			for (int c = 0; c < 3; c++) {
				auto in = this->makeInput(90.f, c == 2 ? "y" : "x", xs[c], y);
				if (r < static_cast<int>(saved.size())) {
					in->setString(saved[r][c]);
				}
				row.push_back(in);
			}
			m_cells.push_back(row);
		}

		// ---- where the wave is right now ----
		std::string info = "Open the pause menu next to the gap to read the wave's x and y here.";
		auto pl = PlayLayer::get();
		if (pl && pl->m_player1) {
			info = fmt::format(
				"Wave now: x {:.1f}    y {:.1f}",
				pl->m_player1->getPositionX(), pl->m_player1->getPositionY()
			);
		}
		this->makeLabel(info.c_str(), "bigFont.fnt", 0.32f, 0.f, -92.f, false);

		this->makeLabel(
			"From (X from - Approach) the wave flies to Y, then spams Hold / Release steps.",
			"bigFont.fnt", 0.25f, 0.f, -108.f, false
		);
		this->makeLabel(
			"1 / 1 = every physics step. 30 units = one block. Empty rows are ignored.",
			"bigFont.fnt", 0.25f, 0.f, -120.f, false
		);

		return true;
	}

	void onEnable(CCObject* sender) {
		bool on = !static_cast<CCMenuItemToggler*>(sender)->isToggled();
		Mod::get()->setSavedValue<bool>("gs-enabled", on);
		g_reload = true;
	}

	void onAlways(CCObject* sender) {
		bool on = !static_cast<CCMenuItemToggler*>(sender)->isToggled();
		Mod::get()->setSavedValue<bool>("gs-always", on);
		g_reload = true;
	}

	void saveFromInputs() {
		auto mod = Mod::get();

		std::string hold = val(m_hold);
		std::string release = val(m_release);
		std::string approach = val(m_approach);
		mod->setSavedValue<std::string>("gs-hold", hold);
		mod->setSavedValue<std::string>("gs-release", release);
		mod->setSavedValue<std::string>("gs-approach", approach);

		std::string out;
		for (auto& row : m_cells) {
			std::string a = val(row[0]);
			std::string b = val(row[1]);
			std::string c = val(row[2]);
			if (a.empty() || b.empty() || c.empty()) continue;

			if (!out.empty()) out += ",";
			out += a;
			out += ":";
			out += b;
			out += ":";
			out += c;
		}
		mod->setSavedValue<std::string>("gs-zones", out);

		g_reload = true;
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

	// everything is saved when the window is closed
	~SpamPopup() {
		this->saveFromInputs();
	}
};

class $modify(SpamPlayer, PlayerObject) {
public:
	struct Fields {
		float lastY = 0.f;
		bool haveLast = false;      // do we know what we pressed last?
		bool lastHold = false;
		bool phaseHold = false;     // spam pattern state
		int phaseCount = 0;
	};

	// press or release, only when the state changes
	void setHold(bool hold) {
		auto f = m_fields.self();
		if (f->haveLast && f->lastHold == hold) return;

		if (hold) {
			this->pushButton(static_cast<PlayerButton>(1));
		} else {
			this->releaseButton(static_cast<PlayerButton>(1));
		}
		f->lastHold = hold;
		f->haveLast = true;
	}

	// one step of the spam: hold N steps, release M steps, repeat
	void spamStep() {
		auto f = m_fields.self();
		if (f->phaseCount <= 0) {
			f->phaseHold = !f->phaseHold;
			f->phaseCount = f->phaseHold ? g_cfg.hold : g_cfg.release;
		}
		f->phaseCount--;
		this->setHold(f->phaseHold);
	}

	void update(float dt) {
		PlayerObject::update(dt);

		if (g_reload) loadConfig();

		auto f = m_fields.self();

		float py = this->getPositionY();
		float stepY = std::fabs(py - f->lastY);   // how far the wave moves in one step
		f->lastY = py;

		if (!g_cfg.enabled) {
			f->haveLast = false;
			f->phaseCount = 0;
			return;
		}

		// autoclicker: spam everywhere
		if (g_cfg.always) {
			this->spamStep();
			return;
		}

		// zones work for the wave
		if (!m_isDart || g_cfg.zones.empty()) {
			f->haveLast = false;
			f->phaseCount = 0;
			return;
		}

		float px = this->getPositionX();

		const Zone* zone = nullptr;
		for (auto& z : g_cfg.zones) {
			if (px >= z.x0 - g_cfg.approach && px <= z.x1) {
				zone = &z;
				break;
			}
		}
		if (!zone) {
			f->haveLast = false;
			f->phaseCount = 0;
			return;
		}

		float err = zone->y - py;
		float tol = std::max(0.05f, stepY * 2.5f);

		// at the right height: spam
		if (std::fabs(err) <= tol) {
			this->spamStep();
			return;
		}

		// not there yet: fly to the height
		f->phaseCount = 0;
		bool wantUp = err > 0.f;
		bool hold = (wantUp == !m_isUpsideDown);
		this->setHold(hold);
	}
};

class $modify(SpamPause, PauseLayer) {
	void customSetup() {
		PauseLayer::customSetup();

		auto win = CCDirector::get()->getWinSize();

		// text button (no image needed)
		CCNode* icon = ButtonSprite::create("Spam");

		// find the menu with the buttons on the right side
		CCMenu* rightMenu = nullptr;
		if (auto byId = typeinfo_cast<CCMenu*>(this->getChildByID("right-button-menu"))) {
			rightMenu = byId;
		} else if (auto kids = this->getChildren()) {
			int best = 0;
			for (auto obj : CCArrayExt<CCNode*>(kids)) {
				auto menu = typeinfo_cast<CCMenu*>(obj);
				if (!menu) continue;
				if (menu->getPositionX() < win.width * 0.6f) continue;
				int n = static_cast<int>(menu->getChildrenCount());
				if (n > best) {
					best = n;
					rightMenu = menu;
				}
			}
		}

		// size like the buttons next to it
		float size = 45.f;
		if (rightMenu && rightMenu->getChildrenCount() > 0) {
			if (auto first = typeinfo_cast<CCNode*>(rightMenu->getChildren()->objectAtIndex(0))) {
				float h = first->getContentSize().height;
				if (h > 10.f) size = h;
			}
		}
		float ch = icon->getContentSize().height;
		if (ch > 1.f) icon->setScale(std::min(1.f, size / ch));

		auto btn = CCMenuItemSpriteExtra::create(
			icon, this, menu_selector(SpamPause::onSpam)
		);
		btn->setID("gap-spam-button"_spr);

		if (rightMenu) {
			float x = 0.f;
			float minY = 0.f;
			float maxY = 0.f;
			bool any = false;
			if (auto kids = rightMenu->getChildren()) {
				for (auto obj : CCArrayExt<CCNode*>(kids)) {
					if (!obj) continue;
					CCPoint p = obj->getPosition();
					if (!any) {
						x = p.x;
						minY = p.y;
						maxY = p.y;
						any = true;
					} else {
						minY = std::min(minY, p.y);
						maxY = std::max(maxY, p.y);
					}
				}
			}

			rightMenu->addChild(btn);

			if (rightMenu->getLayout()) {
				rightMenu->updateLayout();
			} else if (any) {
				float gap = size + 6.f;
				CCPoint below = rightMenu->convertToWorldSpace(CCPoint(x, minY - gap));
				if (below.y > size * 0.6f) {
					btn->setPosition({ x, minY - gap });
				} else {
					btn->setPosition({ x, maxY + gap });
				}
			}
		} else {
			auto menu = CCMenu::create();
			menu->addChild(btn);
			menu->setPosition({ win.width - 110.f, win.height - 40.f });
			this->addChild(menu);
		}
	}

	void onSpam(CCObject*) {
		if (auto p = SpamPopup::create()) p->show();
	}
};
