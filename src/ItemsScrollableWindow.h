#pragma once

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include "ResourceManagers/AssetsManager.h"
#include "ResourceManagers/UIStateManager.h"
#include "ResourceManagers/PreviewManager.h"
#include "ResourceManagers/TextureManager.h"
#include "Things/Items.h"
#include "Things/ThingCategory.h"
#include "Misc/tools.h"

class ItemsScrollableWindow {
public:
    ItemsScrollableWindow(sf::RenderWindow& window, AssetsManager* am);
    // Alternative constructor with direct manager dependencies (for reduced coupling)
    ItemsScrollableWindow(sf::RenderWindow& window, UIStateManager* uiState, PreviewManager* preview, TextureManager* texture, AssetsManager* am);
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
        if(uiStateManager->hasUnsavedChanges(CATEGORY_ITEMS_ITEMTYPE)) {
            shouldOpenUnsavedPopup = true;
            return true;
        }

        return false;
    }

    // ItemType list button methods
    int getTotalButtons() {
        return (int)Items::getItemTypesCount();
    }
    int getSelectedButtonIndex() {
        return selectedItemIndex;
    }
    int isAnyButtonSelected() {
        return selectedItemIndex >= 0 && selectedItemIndex < Items::getItemTypesCount();
    }
    void setSelectedButtonIndex(int id, bool goToSelect = true) {
        if (id < 0 | id > getTotalButtons()) {
            return;
        }

        int oldPage = getCurrentPage();
        int newPage = id / ConfigManager::getInstance()->getButtonsCountItemPage();
        setCurrentPage(newPage);
        
        // If page changed, trigger preview generation
        // Don't auto-select first item since we're already selecting a specific item
        if (oldPage != newPage) {
            onPageChanged(oldPage, newPage, false);
        }

        if(goToSelect) {
            scrollToButtonIndex = id;
        }
        selectedItemIndex = id;
    }

    // Pagination methods
    void drawPaginationControls();
    int getPageFirstIndex() {
        return getCurrentPage() * ConfigManager::getInstance()->getButtonsCountItemPage();;
    }
    int getPageLastIndex() {
        return std::min(getPageFirstIndex() + ConfigManager::getInstance()->getButtonsCountItemPage(), getTotalButtons());
    }
    int getLastPageNumber() {
        return (getTotalButtons() - 1) / ConfigManager::getInstance()->getButtonsCountItemPage();;
    }
    void incrementPage() {
        if(!onPageChange()) {
            return;
        }

        if(getPageLastIndex() >= getTotalButtons()) {
            return;
        }

        int oldPage = getCurrentPage();
        setCurrentPage(oldPage + 1);
        onPageChanged(oldPage, getCurrentPage());
    }
    void decrementPage() {
        if(!onPageChange()) {
            return;
        }

        if(getCurrentPage() <= 0) {
            return;
        }

        int oldPage = getCurrentPage();
        setCurrentPage(oldPage - 1);
        onPageChanged(oldPage, getCurrentPage());
    }
    // True -> you can change page
    bool onPageChange() {
        if(triggerItemSavePrompt()) {
            return false;
        }

        return true;
    }
    void onPageChanged(int oldPage, int newPage, bool autoSelectFirst = true) {
        if(oldPage == newPage) {
            return;
        }

        // Load preview textures for current page
        previewManager->createPreviewTexturesForPage(getPageFirstIndex(), getPageLastIndex(), ThingCategory::ITEM);

        // Only auto-select first item if requested (e.g., from page navigation buttons)
        // Don't auto-select when called from search box (we already have a selected item)
        if (autoSelectFirst) {
            selectItem(getPageFirstIndex());
        }
    }

    void exportItem(Tools::EXPORT_OPTIONS option) {
        if(!isAnyButtonSelected()) {
            return;
        }
        std::string filePath = (std::filesystem::path(outputFolder) / (Tools::trim(itemName))).string() + getFormatString(option);

        auto item = Items::getItemType(getSelectedButtonIndex());

        switch(option) {
            case Tools::PNG:
            case Tools::BMP:
            case Tools::JPG:
                // Export sprite sheet - need to get it from AssetsManager for now
                assetsManager->exportTexture(filePath, assetsManager->getItemSpriteSheet(getSelectedButtonIndex(), item->animationsFrames));
                break;
            case Tools::TOML:
                Items::exportItemToml(filePath, getSelectedButtonIndex());
                break;
            case Tools::ITF:
                Items::exportItemItf(filePath, getSelectedButtonIndex());
                break;
            default:
                Items::exportItemItf(filePath, getSelectedButtonIndex());
                break;
        }
    }

    int getCurrentPage() {
        return currentPage;
    }
    void setCurrentPage(int _newPage) {
        currentPage = _newPage;
    }
private:
    sf::RenderWindow& window;
    AssetsManager* assetsManager; // Still needed for some methods like getItemSpriteSheet, isGraphicFileLoaded, etc.
    UIStateManager* uiStateManager;
    PreviewManager* previewManager;
    TextureManager* textureManager;
    Items* items;

    int currentPage = 0;
    int scrollToButtonIndex = -1;
    inline static int selectedItemIndex = -1;

    char idInputBuffer[10]; // the value of input for searching ItemType on the list
    bool drawGrid = true;

    bool shouldOpenUnsavedPopup = false; // for "Save" changed itemType popup

    // Animation playback
    bool isAnimationPlaying = false;
    sf::Clock animationClock;

    // Variables for export
    std::string outputFolder = Tools::getDesktopPath();
    int exportFormatSelected = 0; // 0 = PNG, 1 = BMP, 2 = JPG, 3 = ITF, 4 = TOML
    std::string itemName = "item" + std::to_string(getSelectedButtonIndex());

    uint32_t rightMenuClickedItem = 0;

    void drawGUIItemTypeExport();
    void handleItemTypeImport();
    void onPostItemImport();
};
