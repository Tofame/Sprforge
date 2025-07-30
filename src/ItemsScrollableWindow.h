#pragma once

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include "ResourceManagers/AssetsManager.h"
#include "Things/Items.h"
#include "Misc/tools.h"

class ItemsScrollableWindow {
public:
    ItemsScrollableWindow(sf::RenderWindow& window, AssetsManager* am);
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

        setCurrentPage(id / ConfigManager::getInstance()->getButtonsCountItemPage());

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
    void onPageChanged(int oldPage, int newPage) {
        if(oldPage == newPage) {
            return;
        }

        // Load preview textures for current page
        assetsManager->createPreviewTexturesForPage(getPageFirstIndex(), getPageLastIndex());

        selectItem(getPageFirstIndex());
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
    AssetsManager* assetsManager;
    Items* items;

    int currentPage = 0;
    int scrollToButtonIndex = -1;
    inline static int selectedItemIndex = -1;

    char idInputBuffer[10]; // the value of input for searching ItemType on the list
    bool drawGrid = true;

    bool shouldOpenUnsavedPopup = false; // for "Save" changed itemType popup

    // Variables for export
    std::string outputFolder = Tools::getDesktopPath();
    int exportFormatSelected = 0; // 0 = PNG, 1 = BMP, 2 = JPG, 3 = ITF, 4 = TOML
    std::string itemName = "item" + std::to_string(getSelectedButtonIndex());

    uint32_t rightMenuClickedItem = 0;

    void drawGUIItemTypeExport();
    void handleItemTypeImport();
    void onPostItemImport();
};
