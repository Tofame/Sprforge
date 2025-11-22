#pragma once

#include "../Misc/definitions.h"
#include <memory>

// Forward declarations
class ItemType;

// ASSET_CATEGORY enum - moved here to avoid circular dependency
// This should ideally be in definitions.h, but keeping it here for now
enum ASSET_CATEGORY {
    CATEGORY_ITEMS = 0,
    CATEGORY_ITEMS_ITEMTYPE = 1,
    CATEGORY_SPRITES = 2,
    CATEGORY_OUTFITS = 3,
    CATEGORY_MAIN_ONES // especially important on changing unsaved changes on - sprites, items, outfits
};

/**
 * @brief Manages UI-specific state that doesn't belong in the data layer
 * 
 * This class separates UI state management from data management,
 * reducing coupling between AssetsManager and UI components.
 */
class UIStateManager {
public:
    UIStateManager();
    ~UIStateManager() = default;

    // Selection state management
    /**
     * @brief Gets the last selected item ID
     * 
     * Used when navigating between items with unsaved changes.
     * If user cancels save prompt, we can restore the selection.
     */
    [[nodiscard]] int getLastSelectedItemId() const { return lastSelectedItemId; }
    void setLastSelectedItemId(int id) { lastSelectedItemId = id; }

    /**
     * @brief Gets the last selected category
     * 
     * Used to determine context for operations like copy/paste.
     */
    ASSET_CATEGORY getLastSelectedCategory() const { return lastSelectedCategory; }
    void setLastSelectedCategory(ASSET_CATEGORY category) { lastSelectedCategory = category; }

    // Animation frame setting (for UI slider)
    [[nodiscard]] int getAnimationFrameSetting() const { return animationFrameSetting; }
    int& getAnimationFrameSettingRef() { return animationFrameSetting; }
    void setAnimationFrameSetting(int id);

    // Unsaved changes tracking
    /**
     * @brief Checks if there are unsaved changes in a category
     * 
     * @param fromCategory Category to check
     * @return True if there are unsaved changes
     */
    [[nodiscard]] bool hasUnsavedChanges(ASSET_CATEGORY fromCategory) const;
    
    /**
     * @brief Sets unsaved changes flag for a category
     * 
     * @param fromCategory Category to set
     * @param value True if there are unsaved changes
     */
    void setUnsavedChanges(ASSET_CATEGORY fromCategory, bool value);

    // Unsaved ItemType management
    /**
     * @brief Sets an unsaved ItemType copy
     * 
     * Used when editing an ItemType. The unsaved copy is stored
     * until the user saves or cancels.
     */
    void setUnsavedItemType(const std::shared_ptr<class ItemType>& itemTypeToCopy, int id);
    void resetUnsavedItemType();
    std::shared_ptr<class ItemType> getUnsavedItemType();
    [[nodiscard]] int getUnsavedItemTypeId() const { return unsavedItemTypeId; }

private:
    // Selection state
    int lastSelectedItemId = -1;
    ASSET_CATEGORY lastSelectedCategory = CATEGORY_ITEMS;

    // Animation frame setting (for UI slider)
    int animationFrameSetting = 1;

    // Unsaved changes tracking
    bool unsavedSpriteChanges = false;
    bool unsavedItemChanges = false;
    bool unsavedItemTypeChange = false;

    // Unsaved ItemType management
    std::shared_ptr<class ItemType> unsavedItemTypeCopy;
    int unsavedItemTypeId = -1;
};

