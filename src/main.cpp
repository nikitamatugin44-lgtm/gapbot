
#include <Geode/Geode.hpp>

#include "PrecisionStorage.hpp"

using namespace geode::prelude;

$on_mod(Loaded) {
    PrecisionStorage storage;

    constexpr PrecisionStorage::ObjectKey testObject = 1;

    storage.setPosition(
        testObject,
        45.0,
        100.0
    );

    // 1000 movements of 0.000000001.
    for (int i = 0; i < 1000; ++i) {
        storage.moveBy(
            testObject,
            0.000000001,
            0.000000001
        );
    }

    auto position = storage.getPosition(testObject);

    if (!position) {
        log::error("Extended Precision: storage test failed!");
        return;
    }

    log::info(
        "Extended Precision: storage test passed!"
    );

    log::info(
        "Exact X: {}",
        PrecisionStorage::formatNumber(position->x)
    );

    log::info(
        "Exact Y: {}",
        PrecisionStorage::formatNumber(position->y)
    );

    log::info(
        "Object count: {}",
        storage.size()
    );
}
