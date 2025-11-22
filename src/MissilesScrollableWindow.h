#pragma once

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include "ResourceManagers/AssetsManager.h"
#include "ResourceManagers/UIStateManager.h"
#include "ResourceManagers/PreviewManager.h"
#include "ResourceManagers/TextureManager.h"
#include "ResourceManagers/ThingTypeHelper.h"
#include "Things/Missiles.h"
#include "Things/ThingType.h"
#include "Things/ThingCategory.h"
#include "Misc/tools.h"
#include "Misc/definitions.h"

class MissilesScrollableWindow {
public:
    MissilesScrollableWindow(sf::RenderWindow& window, AssetsManager* am);
    // Alternative constructor with direct manager dependencies
    MissilesScrollableWindow(sf::RenderWindow& window, UIStateManager* uiState, PreviewManager* preview, TextureManager* texture, AssetsManager* am);
    void drawMissileTypeList(sf::Clock& deltaClock);
    void drawMissileTypePanel();
    void drawPaginationControls();

    void selectMissile(int id, bool goToSelect = true);
    int addMissileType();
    bool removeMissileType();

    int getTotalButtons() { return (int)Missiles::getMissileTypesCount(); }
    int getSelectedButtonIndex() { return selectedMissileIndex; }
    bool isAnyButtonSelected() { return selectedMissileIndex >= 0 && selectedMissileIndex < Missiles::getMissileTypesCount(); }

private:
    sf::RenderWindow& window;
    AssetsManager* assetsManager; // Still needed for isGraphicFileLoaded, isDatFileLoaded
    UIStateManager* uiStateManager;
    PreviewManager* previewManager;
    TextureManager* textureManager;

    int currentPage = 0;
    int scrollToButtonIndex = -1;
    inline static int selectedMissileIndex = -1;

    char idInputBuffer[10];
    bool drawGrid = true;
    std::shared_ptr<MissileType> unsavedMissileType;
    int unsavedMissileTypeId = -1;

    // Animation playback
    bool isAnimationPlaying = false;
    sf::Clock animationClock;

    int getCurrentPage() { return currentPage; }
    void setCurrentPage(int page) { currentPage = page; }
    int getPageFirstIndex() { return getCurrentPage() * ConfigManager::getInstance()->getButtonsCountItemPage(); }
    int getPageLastIndex() { return std::min(getPageFirstIndex() + ConfigManager::getInstance()->getButtonsCountItemPage(), getTotalButtons()); }
};

