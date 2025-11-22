#pragma once

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include "ResourceManagers/AssetsManager.h"
#include "Things/Missiles.h"
#include "Things/ThingType.h"
#include "Misc/tools.h"

class MissilesScrollableWindow {
public:
    MissilesScrollableWindow(sf::RenderWindow& window, AssetsManager* am);
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
    AssetsManager* assetsManager;

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

