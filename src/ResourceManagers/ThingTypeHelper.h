#pragma once

#include "../Things/ThingType.h"
#include <memory>

/**
 * @brief Helper class for ThingType texture ID calculations
 * 
 * This class provides methods to calculate texture IDs from ThingType data
 * based on the Tibia .dat format.
 */
class ThingTypeHelper {
public:
    /**
     * @brief Gets texture id from a ThingType
     *
     * Every ThingType can be composed of more than 1 texture.
     * The sprite index is calculated using the Tibia .dat format order (matching ObjectBuilder):
     * spriteIndex = (((((((frame % frames) * patternZ + patternZ) * patternY + patternY) * patternX + patternX) * layers + layer) * height + height) * width + width
     *
     * @param thingType thingType from which we will get basic information
     * @param w cell's 'width' (0-based) - X coordinate
     * @param h cell's 'height' (0-based) - Y coordinate
     * @param a cell's 'animation frame' (1-based, converted to 0-based internally)
     * @param layer layer index (0-based, defaults to 0)
     * @param patternX pattern X index (0-based, defaults to 0)
     * @param patternY pattern Y index (0-based, defaults to 0)
     * @param patternZ pattern Z index (0-based, defaults to 0)
     */
    static uint32_t getTextureIdFromThingType(const std::shared_ptr<ThingType>& thingType, int w, int h, int a, 
                                              int layer = 0, int patternX = 0, int patternY = 0, int patternZ = 0);

    /**
     * @brief Sets texture id in a ThingType
     *
     * @param thingType thingType that we want to assign texture id to
     * @param w cell's 'width' (0-based) - X coordinate
     * @param h cell's 'height' (0-based) - Y coordinate
     * @param a cell's 'animation frame' (1-based)
     * @param newId textureId that will be set at the desired position
     * @param layer layer index (0-based, defaults to 0)
     * @param patternX pattern X index (0-based, defaults to 0)
     * @param patternY pattern Y index (0-based, defaults to 0)
     * @param patternZ pattern Z index (0-based, defaults to 0)
     */
    static void setTextureIdFromThingType(std::shared_ptr<ThingType> thingType, int w, int h, int a, int newId,
                                          int layer = 0, int patternX = 0, int patternY = 0, int patternZ = 0);
};

