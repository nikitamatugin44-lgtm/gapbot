
#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

#include <chrono>
#include <cstdint>

using namespace geode::prelude;

class $modify(TPSProfiler, GJBaseGameLayer) {

    void update(float dt) {
        using Clock = std::chrono::steady_clock;

        static auto lastReport = Clock::now();
        static uint64_t updateCount = 0;
        static uint64_t totalTimeNs = 0;
        static uint64_t maxTimeNs = 0;

        auto start = Clock::now();

        GJBaseGameLayer::update(dt);

        auto end = Clock::now();

        auto elapsed = std::chrono::duration_cast<
            std::chrono::nanoseconds
        >(end - start).count();

        updateCount++;
        totalTimeNs += elapsed;

        if (static_cast<uint64_t>(elapsed) > maxTimeNs)
            maxTimeNs = elapsed;

        auto period = std::chrono::duration_cast<
            std::chrono::milliseconds
        >(end - lastReport).count();

        if (period >= 1000) {
            double seconds = period / 1000.0;
            double callsPerSecond = updateCount / seconds;
            double averageUs = updateCount
                ? (totalTimeNs / 1000.0) / updateCount
                : 0.0;

            log::info(
                "[TPS Profiler] Updates/sec: {:.1f} | "
                "Average update: {:.2f} us | "
                "Max update: {:.2f} us",
                callsPerSecond,
                averageUs,
                maxTimeNs / 1000.0
            );

            updateCount = 0;
            totalTimeNs = 0;
            maxTimeNs = 0;
            lastReport = end;
        }
    }
};
