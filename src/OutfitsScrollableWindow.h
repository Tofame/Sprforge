#pragma once

#include "Graphics/SFMLCompat.h"
#include <imgui.h>
#include "ThingScrollableWindow.h"
#include "Things/Outfits.h"
#include "Things/ThingType.h"
#include "Misc/tools.h"

class OutfitsScrollableWindow : public ThingScrollableWindow {
public:
    explicit OutfitsScrollableWindow(AssetsManager* am);
    
    // Implement pure virtual methods from base class
    void drawTypeList(sf::Clock& deltaClock) override { drawOutfitTypeList(deltaClock); }
    void drawTypePanel() override { drawOutfitTypePanel(); }
    void selectType(int id, bool goToSelect = true) override { selectOutfit(id, goToSelect); }
    int addType() override { return addOutfitType(); }
    bool removeType() override { return removeOutfitType(); }
    
    // Type-specific methods (wrappers for base class virtual methods)
    void drawOutfitTypeList(sf::Clock& deltaClock);
    void drawOutfitTypePanel();
    void selectOutfit(int id, bool goToSelect = true);
    int addOutfitType();
    bool removeOutfitType();

    // Override base class methods
    int getTotalButtons() const override { return (int)Outfits::getOutfitTypesCount(); }
    int getSelectedButtonIndex() override { return selectedOutfitIndex; }
    bool isAnyButtonSelected() override { return selectedOutfitIndex >= 0 && selectedOutfitIndex < Outfits::getOutfitTypesCount(); }

private:
    inline static int selectedOutfitIndex = -1;
    std::shared_ptr<OutfitType> unsavedOutfitType;
    int unsavedOutfitTypeId = -1;
    
    // Direction selection (0=North, 1=East, 2=South, 3=West)
    int selectedDirection = 0;
};

