#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "ThingCategory.h"

struct LightBlock {
    bool hasLight = 0;
    uint8_t lightColor = 0;
    uint8_t lightIntensity = 0;  
};

// Base class for all thing types (items, outfits, effects, missiles)
class ThingType {
public:
    ThingType();
    virtual ~ThingType() = default;
    ThingType(const ThingType&) = default;
    ThingType& operator=(const ThingType&) = default;

    ThingCategory category = ThingCategory::ITEM;
    
    // Sprite data
    std::vector<uint32_t> textureIdsVector;
    uint8_t width = 1;
    uint8_t height = 1;
    uint8_t animationsFrames = 1;
    uint8_t patternX = 1;
    uint8_t patternY = 1;
    uint8_t patternZ = 1;
    uint8_t layers = 1;

    // Properties that all child classes have
    LightBlock lightBlock;

    // Common methods
    void setWidth(int width);
    void setHeight(int height);
    void setAnimationCount(int count);
    void setLayers(int layers);
    void setPatternX(int patternX);
    void setPatternY(int patternY);
    void setPatternZ(int patternZ);

    [[nodiscard]] int getCalcIndexesCount() const;

    bool operator==(const ThingType& other) const;
    bool operator!=(const ThingType& other) const;

protected:
    void updateTextureVectorSize();
};

