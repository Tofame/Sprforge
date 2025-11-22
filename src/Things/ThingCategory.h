#pragma once

enum class ThingCategory {
    ITEM = 1,
    OUTFIT = 2,
    EFFECT = 3,
    MISSILE = 4
};

inline const char* ThingCategoryToString(ThingCategory category) {
    switch (category) {
        case ThingCategory::ITEM: return "item";
        case ThingCategory::OUTFIT: return "outfit";
        case ThingCategory::EFFECT: return "effect";
        case ThingCategory::MISSILE: return "missile";
        default: return "unknown";
    }
}

