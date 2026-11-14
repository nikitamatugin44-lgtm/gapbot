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
class
