#pragma once

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include "ThingScrollableWindow.h"
#include "Things/Effects.h"
#include "Things/ThingType.h"
#include "Misc/tools.h"

class EffectsScrollableWindow : public ThingScrollableWindow {
public:
    EffectsScrollableWindow(sf::RenderWindow& window, AssetsManager* am);
    
    // Implement pure virtual methods from base class
    void drawTypeList(sf::Clock& deltaClock) override { drawEffectTypeList(deltaClock); }
    void drawTypePanel() override { drawEffectTypePanel(); }
    void selectType(int id, bool goToSelect = true) override { selectEffect(id, goToSelect); }
    int addType() override { return addEffectType(); }
    bool removeType() override { return removeEffectType(); }
    
    // Type-specific methods (wrappers for base class virtual methods)
    void drawEffectTypeList(sf::Clock& deltaClock);
    void drawEffectTypePanel();
    void selectEffect(int id, bool goToSelect = true);
    int addEffectType();
    bool removeEffectType();

    // Override base class methods
    int getTotalButtons() const override { return (int)Effects::getEffectTypesCount(); }
    int getSelectedButtonIndex() override { return selectedEffectIndex; }
    bool isAnyButtonSelected() override { return selectedEffectIndex >= 0 && selectedEffectIndex < Effects::getEffectTypesCount(); }

private:
    inline static int selectedEffectIndex = -1;
    std::shared_ptr<EffectType> unsavedEffectType;
    int unsavedEffectTypeId = -1;
};

