#pragma once

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include "ResourceManagers/AssetsManager.h"
#include "Things/Effects.h"
#include "Things/ThingType.h"
#include "Misc/tools.h"

class EffectsScrollableWindow {
public:
    EffectsScrollableWindow(sf::RenderWindow& window, AssetsManager* am);
    void drawEffectTypeList(sf::Clock& deltaClock);
    void drawEffectTypePanel();
    void drawPaginationControls();

    void selectEffect(int id, bool goToSelect = true);
    int addEffectType();
    bool removeEffectType();

    int getTotalButtons() { return (int)Effects::getEffectTypesCount(); }
    int getSelectedButtonIndex() { return selectedEffectIndex; }
    bool isAnyButtonSelected() { return selectedEffectIndex >= 0 && selectedEffectIndex < Effects::getEffectTypesCount(); }

private:
    sf::RenderWindow& window;
    AssetsManager* assetsManager;

    int currentPage = 0;
    int scrollToButtonIndex = -1;
    inline static int selectedEffectIndex = -1;

    char idInputBuffer[10];
    bool drawGrid = true;
    std::shared_ptr<EffectType> unsavedEffectType;
    int unsavedEffectTypeId = -1;

    int getCurrentPage() { return currentPage; }
    void setCurrentPage(int page) { currentPage = page; }
    int getPageFirstIndex() { return getCurrentPage() * ConfigManager::getInstance()->getButtonsCountItemPage(); }
    int getPageLastIndex() { return std::min(getPageFirstIndex() + ConfigManager::getInstance()->getButtonsCountItemPage(), getTotalButtons()); }
};

