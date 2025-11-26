#pragma once

#include "Graphics/SFMLCompat.h"
#include <imgui.h>
#include "ThingScrollableWindow.h"
#include "Things/Missiles.h"
#include "Things/ThingType.h"
#include "Misc/tools.h"

class MissilesScrollableWindow : public ThingScrollableWindow {
public:
    explicit MissilesScrollableWindow(AssetsManager* am);
    
    // Implement pure virtual methods from base class
    void drawTypeList(sf::Clock& deltaClock) override { drawMissileTypeList(deltaClock); }
    void drawTypePanel() override { drawMissileTypePanel(); }
    void selectType(int id, bool goToSelect = true) override { selectMissile(id, goToSelect); }
    int addType() override { return addMissileType(); }
    bool removeType() override { return removeMissileType(); }
    
    // Type-specific methods (wrappers for base class virtual methods)
    void drawMissileTypeList(sf::Clock& deltaClock);
    void drawMissileTypePanel();
    void selectMissile(int id, bool goToSelect = true);
    int addMissileType();
    bool removeMissileType();

    // Override base class methods
    int getTotalButtons() const override { return (int)Missiles::getMissileTypesCount(); }
    int getSelectedButtonIndex() override { return selectedMissileIndex; }
    bool isAnyButtonSelected() override { return selectedMissileIndex >= 0 && selectedMissileIndex < Missiles::getMissileTypesCount(); }

private:
    inline static int selectedMissileIndex = -1;
    std::shared_ptr<MissileType> unsavedMissileType;
    int unsavedMissileTypeId = -1;
};

