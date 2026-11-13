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
	float size = 0.f;
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
		if (r.size.width < 0.5f || r.size.height < 0.5f) continue;
		if (r.origin.x + r.size.width < px - 30.f) continue;
		if (r.origin.x > px + range) continue;

		g_blocks.push_back(Block{
			r.origin.x, r.origin.x + r.size.width,
			r.origin.y, r.origin.y + r.size.height
		});
		if (g_blocks.size() >= 150) break;
	}
}

// true if some other block sits inside the gap and closes it
static bool gapIsBlocked(const Block& lo, const Block& hi, float ox0, float ox1) {
	for (auto& b : g_blocks) {
		if (&b == &lo || &b == &hi) continue;
		if (b.x1 <= ox0 || b.x0 >= ox1) continue;
		if (b.y1 <= lo.y1 + 0.01f || b.y0 >= hi.y0 - 0.01f) continue;
		return true;
	}
	return false;
}

// finds the nearest narrow gap between a lower and an upper block.
// It uses only the blocks, so it does not matter where the wave is.
static Gap findGap(float px, float half, float range, float maxGap) {
	Gap best;
	float minGap = half * 2.f;   // smaller than the wave: impossible

	for (auto& lo : g_blocks) {
		if (lo.x1 < px - half || lo.x0 > px + range) continue;

		for (auto& hi : g_blocks) {
			if (hi.x1 < px - half || hi.x0 > px + range) continue;

			float size = hi.y0 - lo.y1;            // empty space between them
			if (size < minGap || size > maxGap) continue;

			// both blocks must exist at the same x for a few units
			float ox0 = std::max(lo.x0, hi.x0);
			float ox1 = std::min(lo.x1, hi.x1);
			if (ox1 - ox0 < 2.f) continue;
			if (ox1 < px - half) continue;

			if (gapIsBlocked(lo, hi, ox0, ox1)) continue;

			// take the gap that starts first
			if (!best.ok || ox0 < best.x0) {
				best.ok = true;
				best.center = (hi.y0 + lo.y1) * 0.5f;
				best.size = size;
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
		float lastLogX = -1.f;
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

		rebuildBlocks(px, range + 100.f);
		f->gap = findGap(px, half, range, maxGap);

		// debug: tell about every new gap once
		if (f->gap.ok && std::fabs(f->gap.x0 - f->lastLogX) > 1.f) {
			f->lastLogX = f->gap.x0;
			log::info(
				"Gap Spam: gap x {:.1f}..{:.1f}, center y {:.1f}, size {:.1f}, wave y {:.1f}",
				f->gap.x0, f->gap.x1, f->gap.center, f->gap.size, this->getPositionY()
			);
		}
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
