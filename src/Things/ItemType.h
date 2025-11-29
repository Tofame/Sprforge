#pragma once

#include "ThingType.h"
#include <cstdint>
#include <string>

enum ItemCategory_t {
    COMMON = 0,
    GROUND_BORDER = 1,
    BOTTOM = 2,
    TOP = 3,
};

enum ItemTypeFlags : uint32_t {
    IS_GROUND      = 1 << 0,
    IS_CONTAINER   = 1 << 1,
    STACKABLE      = 1 << 2,
    FORCE_USE      = 1 << 3,
    MULTI_USE      = 1 << 4,
    UNPASSABLE     = 1 << 5,
    UNMOVABLE      = 1 << 6,
    PICKUPABLE     = 1 << 7,
    BLOCK_MISSILE  = 1 << 8,
};

// ItemType inherits from ThingType and adds item-specific properties
class ItemType : public ThingType
{
public:
    ItemType();
    virtual ~ItemType() = default;
    ItemType(const ItemType&) = default;
    ItemType& operator=(const ItemType&) = default;

    std::string name;
    uint16_t speed = 0;
    uint16_t minimapColor = 0; // Minimap color (0-255)
    ItemCategory_t itemCategory = COMMON;

    // Wrapper methods for backward compatibility
    void setItemTypeWidth(int width) { setWidth(width); }
    void setItemTypeHeight(int height) { setHeight(height); }
    void setItemTypeAnimationCount(int count) { setAnimationCount(count); }
    void setItemTypeLayers(int layers) { setLayers(layers); }
    void setItemTypePatternX(int patternX) { setPatternX(patternX); }
    void setItemTypePatternY(int patternY) { setPatternY(patternY); }
    void setItemTypePatternZ(int patternZ) { setPatternZ(patternZ); }

    bool operator==(const ItemType& other) const {
        return ThingType::operator==(other) &&
               itemTypeFlags == other.itemTypeFlags &&
               speed == other.speed &&
               minimapColor == other.minimapColor &&
               itemCategory == other.itemCategory &&
               name == other.name;
    }

    bool operator!=(const ItemType& other) const {
        return !(*this == other);
    }

    void setFlag(ItemTypeFlags flag, bool enable) {
        if (enable) {
            itemTypeFlags |= flag;
        } else {
            itemTypeFlags &= ~flag;
        }
    }

    [[nodiscard]] bool hasFlag(ItemTypeFlags flag) const {
        return itemTypeFlags & flag;
    }

    [[nodiscard]] uint32_t getAllFlags() const {
        return itemTypeFlags;
    }
    
    void setAllFlags(uint32_t flags) {
        itemTypeFlags = flags;
    }

private:
    uint32_t itemTypeFlags = 0;
};