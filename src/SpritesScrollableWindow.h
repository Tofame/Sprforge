#pragma once

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include "ResourceManagers/AssetsManager.h"
#include "Misc/tools.h"
#include "Helper/DropManager.h"


class SpritesScrollableWindow {
public:
    SpritesScrollableWindow(sf::RenderWindow& window, AssetsManager* am);
    // Draws ScrollablePanel with Textures
    void drawTextureList(sf::Clock& deltaClock);
    // Pagination, Adding/Removing texture etc.
    void drawListControlButtons();

    // Creates new texture, at the last index
    void createNewTexture();
    /**
     * @brief Imports new texture, at the last index
     *
     *
     * @param filePath path to texture
     * If filePath is empty, then it just adds new, blank texture.
     */
    void importTexture(const std::string& filePath);

    /**
     * @brief Select a texture
     *
     * This the main method of selecting the texture from the list
     *
     * @param id id of texture to select
     * @param goToSelect if we list's GUI should scroll down/up to the selected texture
     */
    void selectSprite(int id, bool goToSelect = true);

    // Texture list button methods
    int getTotalButtons() {
        return (int)assetsManager->getTextureCount();
    }
    int getSelectedSpriteIndex() {
        return selectedButtonIndex;
    }
    int isAnySpriteSelected() {
        return selectedButtonIndex >= 0 && selectedButtonIndex < assetsManager->getTextureCount();
    }
    void setSelectedButtonIndex(int id, bool goToSelect = true) {
        if (id < 0 | id > getTotalButtons()) {
            return;
        }

        currentPage = id / ConfigManager::getInstance()->getButtonsCountSpritePage();

        if(goToSelect) {
            scrollToButtonIndex = id;
        }
        selectedButtonIndex = id;
    }

    // Pagination Helper Methods
    int getPageFirstIndex() {
        return currentPage * ConfigManager::getInstance()->getButtonsCountSpritePage();
    }
    int getPageLastIndex() {
        return std::min(getPageFirstIndex() + ConfigManager::getInstance()->getButtonsCountSpritePage(), getTotalButtons());
    }
    void incrementPage() {
        if(!onPageChange()) {
            return;
        }

        if(getPageLastIndex() >= getTotalButtons()) {
            return;
        }

        int oldPage = currentPage;
        currentPage++;
        onPageChanged(oldPage, currentPage);
    }
    void decrementPage() {
        if(!onPageChange()) {
            return;
        }

        if(currentPage <= 0) {
            return;
        }

        int oldPage = currentPage;
        currentPage--;
        onPageChanged(oldPage, currentPage);
    }
    // True -> you can change page
    bool onPageChange() {
        return true;
    }
    void onPageChanged(int oldPage, int newPage) {
        if(oldPage == newPage) {
            return;
        }

        selectSprite(getPageFirstIndex());
    }

    bool hasUnsavedChanges() const { return assetsManager->hasUnsavedChanges(CATEGORY_SPRITES); };
    void setUnsavedChanges(bool value = true) const { assetsManager->setUnsavedChanges(CATEGORY_SPRITES, value); };

    void exportTexture(Tools::EXPORT_OPTIONS option);

    void setDropManager(DropManager* _dm) {
        this->dropManager = _dm;
    }
private:
    sf::RenderWindow& window;
    AssetsManager* assetsManager;
    DropManager* dropManager;

    int currentPage = 0;
    int scrollToButtonIndex = -1;
    int selectedButtonIndex = -1;

    char idInputBuffer[10];

    std::string outputFolder = Tools::getDesktopPath();
    int formatSelected = 0; // 0 = PNG, 1 = BMP, 2 = JPG
    std::string spriteName = "sprite" + std::to_string(getSelectedSpriteIndex());

    uint32_t rightMenuClickedSprite = 0;
};