#include "UIStateManager.h"
#include "../Things/ItemType.h"
#include "../Things/Items.h"
#include "ConfigManager.h"
#include "../Misc/definitions.h"

UIStateManager::UIStateManager() = default;

void UIStateManager::setAnimationFrameSetting(int id) {
    if (id < 0 || id > ConfigManager::getInstance()->getItemMaxAnimationCount()) {
        return;
    }
    animationFrameSetting = id;
}

bool UIStateManager::hasUnsavedChanges(ASSET_CATEGORY fromCategory) const {
    switch (fromCategory) {
        case CATEGORY_SPRITES:
            return unsavedSpriteChanges;
        case CATEGORY_ITEMS:
            return unsavedItemChanges;
        case CATEGORY_ITEMS_ITEMTYPE:
            return unsavedItemTypeChange;
        case CATEGORY_OUTFITS:
            // TODO: Add outfit unsaved changes tracking when implemented
            return false;
        case CATEGORY_MAIN_ONES:
            // Returns true if any main category has unsaved changes
            return unsavedSpriteChanges || unsavedItemChanges || unsavedItemTypeChange;
        default:
            return false;
    }
}

void UIStateManager::setUnsavedChanges(ASSET_CATEGORY fromCategory, bool value) {
    switch (fromCategory) {
        case CATEGORY_SPRITES:
            unsavedSpriteChanges = value;
            break;
        case CATEGORY_ITEMS:
            unsavedItemChanges = value;
            break;
        case CATEGORY_ITEMS_ITEMTYPE:
            unsavedItemTypeChange = value;
            break;
        case CATEGORY_OUTFITS:
            // TODO: Add outfit unsaved changes tracking when implemented
            break;
        case CATEGORY_MAIN_ONES:
            // Sets all main categories to the same value
            unsavedSpriteChanges = value;
            unsavedItemChanges = value;
            unsavedItemTypeChange = value;
            break;
        default:
            break;
    }
}

void UIStateManager::setUnsavedItemType(const std::shared_ptr<ItemType>& itemTypeToCopy, int id) {
    if (itemTypeToCopy) {
        unsavedItemTypeCopy = std::make_shared<ItemType>(*itemTypeToCopy); // deep copy
        unsavedItemTypeId = id;
    } else {
        resetUnsavedItemType();
    }
}

void UIStateManager::resetUnsavedItemType() {
    unsavedItemTypeCopy.reset();
    unsavedItemTypeId = -1;
}

std::shared_ptr<ItemType> UIStateManager::getUnsavedItemType() {
    if (unsavedItemTypeCopy) {
        return unsavedItemTypeCopy;
    } else {
        // Create a brand new copy of dollItemType
        return std::make_shared<ItemType>(*Items::dollItemType);
    }
}

