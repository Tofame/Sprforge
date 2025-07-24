#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum ItemCategory_t {
	COMMON = 0,
	GROUND_BORDER = 1,
	BOTTOM = 2,
	TOP = 3,
};

enum WeaponType_t
{
	WEAPON_NONE = 0,
	WEAPON_SWORD,
	WEAPON_GLOVES
};

enum SlotType_t
{
	SLOT_ANYWHERE = 0,
	SLOT_FIRST = 1,
	SLOT_HEAD = SLOT_FIRST,
	SLOT_NECKLACE = 2,
	SLOT_BACKPACK = 3,
	SLOT_ARMOR = 4,
	SLOT_RHAND = 5,
	SLOT_LHAND = 6,
	SLOT_LEGS = 7,
	SLOT_FEET = 8,
	SLOT_RING = 9,
	SLOT_AMMO = 10,
	SLOT_LAST = SLOT_AMMO
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

class ItemType
{
public:
    ItemType();
    virtual ~ItemType() = default;
    ItemType(const ItemType&) = default;
    std::string name;

    uint16_t speed = 0;
    ItemCategory_t category = COMMON;

    std::vector<uint32_t> textureIdsVector;
    uint8_t width = 1;
    uint8_t height = 1;
    uint8_t animationsFrames = 1;

    uint8_t patternX = 1;
    uint8_t patternY = 1;
    uint8_t patternZ = 1;
    uint8_t layers = 1;

    void setItemTypeWidth(int width);
    void setItemTypeHeight(int height);
    void setItemTypeAnimationCount(int count);

    [[nodiscard]] int getCalcIndexesCount() const;

    bool operator==(const ItemType& other) const {
        return itemTypeFlags == other.itemTypeFlags &&
               speed == other.speed &&
               category == other.category &&
               width == other.width &&
               height == other.height &&
               animationsFrames == other.animationsFrames &&
               textureIdsVector == other.textureIdsVector;
    }

    bool operator!=(const ItemType& other) const {
        return !(*this == other);
    }

    // Copy Assignment Operator
    ItemType& operator=(const ItemType& other) {
        if (this != &other) { // Prevent self-assignment
            itemTypeFlags = other.itemTypeFlags;
            speed = other.speed;
            category = other.category;
            width = other.width;
            height = other.height;
            animationsFrames = other.animationsFrames;
            textureIdsVector = other.textureIdsVector;
        }
        return *this;
    }

    void setFlag(ItemTypeFlags flag, bool enable) {
        if (enable) {
            itemTypeFlags |= flag;  // Set the bit to 1
        } else {
            itemTypeFlags &= ~flag; // Set the bit to 0
        }
    }

    [[nodiscard]] bool hasFlag(ItemTypeFlags flag) const {
        return itemTypeFlags & flag;
    }

    [[nodiscard]] const uint32_t& getAllFlags() const {
        return itemTypeFlags;
    }
    void setAllFlags(uint32_t flags) {
        itemTypeFlags = flags;
    }
private:
    void onItemTypeWidthChanged(int oldWith, int width);
    void onItemTypeHeightChanged(int oldHeight, int height);
    void onItemTypeAnimationFramesChanged(int oldAnimationCount, int animationCount);

    uint32_t itemTypeFlags = 0;
};