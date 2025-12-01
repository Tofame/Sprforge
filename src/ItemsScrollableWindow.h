#pragma once

#include "Graphics/SFMLCompat.h"
#include <imgui.h>
#include "ThingScrollableWindow.h"
#include "Things/Items.h"
#include "Misc/tools.h"

class ItemsScrollableWindow : public ThingScrollableWindow {
public:
    explicit ItemsScrollableWindow(AssetsManager* am);
    
    // Implement pure virtual methods from base class
    void drawTypeList(sf::Clock& deltaClock) override { drawItemTypeList(deltaClock); }
    void drawTypePanel() override { drawItemTypePanel(); }
    void selectType(int id, bool goToSelect = true) override { selectItem(id, goToSelect); }
    int addType() override { return addItemType(); }
    bool removeType() override { return removeItemType(); }
    
    // Override drawPaginationControls to add Export/Import buttons
    void drawPaginationControls() override;
    
    // Override onPageChange to check for unsaved changes
    bool onPageChange() override;
    
    // Type-specific methods (wrappers for base class virtual methods)
    void drawItemTypeList(sf::Clock& deltaClock);
    void drawItemTypePanel();

    /**
     * @brief Select an item
     *
     * This the main method of selecting the itemType from the list
     *
     * @param id id of itemType to select
     * @param goToSelect if we list's GUI should scroll down/up to the selected item
     */
    void selectItem(int id, bool goToSelect = true);

    /**
     * @brief Add new ItemType (empty) to the scrollable list
     *
     * Adds new item type to the list,
     * unless there are unsaved changes.
     *
     * @return Returns index of the newly added item type (-1 if couldn't add)
     */
    int addItemType();
    bool removeItemType();

    // Returns:
    // True - unsaved itemType changes
    // False - everything up-to-date
    bool triggerItemSavePrompt() {
        if(assetsManager->hasUnsavedChanges(CATEGORY_ITEMS_ITEMTYPE)) {
            shouldOpenUnsavedPopup = true;
            return true;
        }

        return false;
    }

    // Override base class methods
    int getTotalButtons() const override {
        return (int)Items::getItemTypesCount();
    }
    int getSelectedButtonIndex() override {
        return selectedItemIndex;
    }
    bool isAnyButtonSelected() override {
        return selectedItemIndex >= 0 && selectedItemIndex < (int)Items::getItemTypesCount();
    }
    
    // Items-specific pagination methods
    int getLastPageNumber() {
        return (getTotalButtons() - 1) / ConfigManager::getInstance()->getButtonsCountItemPage();
    }
    void onPageChanged(int oldPage, int newPage, bool autoSelectFirst = true) override {
        if(oldPage == newPage) {
            return;
        }

        // Load preview textures for current page
        assetsManager->createPreviewTexturesForPage(getPageFirstIndex(), getPageLastIndex(), ThingCategory::ITEM);

        // Only auto-select first item if requested (e.g., from page navigation buttons)
        // Don't auto-select when called from search box (we already have a selected item)
        if (autoSelectFirst) {
            selectItem(getPageFirstIndex());
        }
    }

    void exportItem(Tools::EXPORT_OPTIONS option);
    void setSelectedButtonIndex(int id, bool goToSelect = true);
    
private:
    inline static int selectedItemIndex = -1;
    bool shouldOpenUnsavedPopup = false; // for "Save" changed itemType popup

    // Variables for export
    std::string outputFolder = Tools::getDesktopPath();
    int exportFormatSelected = 0; // 0 = PNG, 1 = BMP, 2 = JPG, 3 = ITF, 4 = TOML
    std::string itemName;

    uint32_t rightMenuClickedItem = 0;

    void drawGUIItemTypeExport();
    void handleItemTypeImport();
    void onPostItemImport();
};
