#pragma once

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include "ResourceManagers/AssetsManager.h"
#include "Things/Outfits.h"
#include "Things/ThingType.h"
#include "Misc/tools.h"

class OutfitsScrollableWindow {
public:
    OutfitsScrollableWindow(sf::RenderWindow& window, AssetsManager* am);
    void drawOutfitTypeList(sf::Clock& deltaClock);
    void drawOutfitTypePanel();
    void drawPaginationControls();

    void selectOutfit(int id, bool goToSelect = true);
    int addOutfitType();
    bool removeOutfitType();

    int getTotalButtons() { return (int)Outfits::getOutfitTypesCount(); }
    int getSelectedButtonIndex() { return selectedOutfitIndex; }
    bool isAnyButtonSelected() { return selectedOutfitIndex >= 0 && selectedOutfitIndex < Outfits::getOutfitTypesCount(); }

private:
    sf::RenderWindow& window;
    AssetsManager* assetsManager;

    int currentPage = 0;
    int scrollToButtonIndex = -1;
    inline static int selectedOutfitIndex = -1;

    char idInputBuffer[10];
    bool drawGrid = true;
    std::shared_ptr<OutfitType> unsavedOutfitType;
    int unsavedOutfitTypeId = -1;

    int getCurrentPage() { return currentPage; }
    void setCurrentPage(int page) { currentPage = page; }
    int getPageFirstIndex() { return getCurrentPage() * ConfigManager::getInstance()->getButtonsCountItemPage(); }
    int getPageLastIndex() { return std::min(getPageFirstIndex() + ConfigManager::getInstance()->getButtonsCountItemPage(), getTotalButtons()); }
};

