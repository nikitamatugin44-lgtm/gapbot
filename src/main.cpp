#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace geode::prelude;

struct Block {
	float x0, x1, y0, y1;   // edges of a solid block
};

struct Gap {
	bool ok = false;
	float center = 0.f;     // height of the middle of the gap
	float x0 = 0.f;         // where the gap starts / ends along x
	float x1 = 0.f;
};

static std::vector<Block> g_blocks;

// collects the solid blocks around the player
static void rebuildBlocks(float px, float range) {
	g_blocks.clear();

	auto pl = PlayLayer::get();
	if (!pl || !pl->m_objects) return;

	for (auto obj : CCArrayExt<GameObject*>(pl->m_objects)) {
		if (!obj) continue;
		if (obj->m_objectType != GameObjectType::Solid) continue;

		CCRect r = obj->getObjectRect();
		if (r.origin.x + r.size.width < px - 30.f) continue;
		if (r.origin.x > px + range) continue;

		g_blocks.push_back(Block{
			r.origin.x, r.origin.x + r.size.width,
			r.origin.y, r.origin.y + r.size.height
		});
		if (g_blocks.size() >= 300) break;
	}
}

// finds the nearest gap: a block above and a block below that overlap along x
static Gap findGap(float px, float py, float half, float range, float maxGap) {
	Gap best;
	float minGap = half * 2.f;   // smaller than the wave: impossible

	for (auto& up : g_blocks) {
		if ((up.y0 + up.y1) * 0.5f <= py) continue;      // must be above the wave
		if (up.x1 < px - half || up.x0 > px + range) continue;

		for (auto& dn : g_blocks) {
			if ((dn.y0 + dn.y1) * 0.5f > py) continue;   // must be below the wave
			if (dn.x1 < px - half || dn.x0 > px + range) continue;

			// both blocks must exist at the same x for a few units
			float ox0 = std::max(up.x0, dn.x0);
			float ox1 = std::min(up.x1, dn.x1);
			if (ox1 - ox0 < 2.f) continue;
			if (ox1 < px - half) continue;

			float size = up.y0 - dn.y1;
			if (size < minGap || size > maxGap) continue;

			// take the gap that starts first
			if (!best.ok || ox0 < best.x0) {
				best.ok = true;
				best.center = (up.y0 + dn.y1) * 0.5f;
				best.x0 = ox0;
				best.x1 = ox1;
			}
		}
	}
	return best;
}

class $modify(GapPlayer, PlayerObject) {
public:
	struct Fields {
		float acc = 1000.f;      // game time since the last refresh
		bool enabled = false;
		float approach = 80.f;
		Gap gap;
		bool haveLast = false;
		bool lastHold = false;
	};

	// settings, blocks and gap are refreshed rarely, not on every step
	void refresh() {
		auto f = m_fields.self();
		auto mod = Mod::get();

		f->gap.ok = false;
		f->enabled = mod->getSettingValue<bool>("gap-enabled");
		if (!f->enabled || !m_isDart) return;

		auto pl = PlayLayer::get();
		if (!pl) return;

		if (mod->getSettingValue<bool>("gap-percent-on")) {
			float pct = pl->getCurrentPercent();
			float a = static_cast<float>(mod->getSettingValue<double>("gap-percent-from"));
			float b = static_cast<float>(mod->getSettingValue<double>("gap-percent-to"));
			if (pct < a || pct > b) return;
		}

		float maxGap = static_cast<float>(mod->getSettingValue<double>("gap-max-size"));
		float base = static_cast<float>(mod->getSettingValue<double>("gap-half"));
		f->approach = static_cast<float>(mod->getSettingValue<double>("gap-approach"));

		float half = base * std::max(0.5f, this->getScale());
		float range = f->approach + 300.f;

		float px = this->getPositionX();
		float py = this->getPositionY();

		rebuildBlocks(px, range + 100.f);
		f->gap = findGap(px, py, half, range, maxGap);
	}

	void update(float dt) {
		PlayerObject::update(dt);

		auto f = m_fields.self();

		f->acc += dt;
		if (f->acc >= 0.02f) {
			f->acc = 0.f;
			this->refresh();
		}

		if (!f->enabled || !m_isDart || !f->gap.ok) {
			f->haveLast = false;
			return;
		}

		float px = this->getPositionX();
		float py = this->getPositionY();

		// only while the wave is approaching or inside the gap
		if (px < f->gap.x0 - f->approach || px > f->gap.x1) {
			f->haveLast = false;
			return;
		}

		// below the middle: go up, above the middle: go down
		bool wantUp = py < f->gap.center;
		bool hold = (wantUp == !m_isUpsideDown);

		if (!f->haveLast || hold != f->lastHold) {
			if (hold) {
				this->pushButton(static_cast<PlayerButton>(1));
			} else {
				this->releaseButton(static_cast<PlayerButton>(1));
			}
			f->lastHold = hold;
			f->haveLast = true;
		}
	}
};
