#include "ThingTypeHelper.h"
#include "../Things/ThingType.h"

uint32_t ThingTypeHelper::getTextureIdFromThingType(const std::shared_ptr<ThingType>& thingType, int w, int h, int a, 
                                                     int layer, int patternX, int patternY, int patternZ) {
    if (!thingType || thingType->width == 0 || thingType->height == 0 || 
        thingType->layers == 0 || thingType->patternX == 0 || thingType->patternY == 0 || 
        thingType->patternZ == 0 || thingType->animationsFrames == 0) {
        return 0; // Return 0 (blank texture) for invalid thingType
    }
    
    // Validate indices are within bounds
    if (w < 0 || w >= thingType->width || h < 0 || h >= thingType->height ||
        layer < 0 || layer >= thingType->layers ||
        patternX < 0 || patternX >= thingType->patternX ||
        patternY < 0 || patternY >= thingType->patternY ||
        patternZ < 0 || patternZ >= thingType->patternZ) {
        return 0; // Return 0 (blank texture) for out of bounds
    }
    
    // Convert animation frame from 1-based to 0-based
    int frame = a - 1;
    if (frame < 0) frame = 0;
    if (frame >= thingType->animationsFrames) frame = frame % thingType->animationsFrames;
    
    // Calculate sprite index using Tibia .dat format
    int spriteIndex = (((((((frame % thingType->animationsFrames) * thingType->patternZ + patternZ) * 
                          thingType->patternY + patternY) * 
                          thingType->patternX + patternX) * 
                          thingType->layers + layer) * 
                          thingType->height + h) * 
                          thingType->width + w);
    
    if (spriteIndex >= 0 && spriteIndex < static_cast<int>(thingType->textureIdsVector.size())) {
        return thingType->textureIdsVector[spriteIndex];
    }
    
    return 0; // Return 0 (blank texture) if index is out of bounds
}

void ThingTypeHelper::setTextureIdFromThingType(std::shared_ptr<ThingType> thingType, int w, int h, int a, int newId,
                                                 int layer, int patternX, int patternY, int patternZ) {
    if (!thingType || thingType->width == 0 || thingType->height == 0 || 
        thingType->layers == 0 || thingType->patternX == 0 || thingType->patternY == 0 || 
        thingType->patternZ == 0 || thingType->animationsFrames == 0) {
        return; // Skip invalid thingType
    }
    
    // Validate indices are within bounds
    if (w < 0 || w >= thingType->width || h < 0 || h >= thingType->height ||
        layer < 0 || layer >= thingType->layers ||
        patternX < 0 || patternX >= thingType->patternX ||
        patternY < 0 || patternY >= thingType->patternY ||
        patternZ < 0 || patternZ >= thingType->patternZ) {
        return; // Skip out of bounds indices
    }
    
    // Convert animation frame from 1-based to 0-based
    int frame = a - 1;
    if (frame < 0) frame = 0;
    if (frame >= thingType->animationsFrames) frame = frame % thingType->animationsFrames;
    
    // Calculate sprite index using Tibia .dat format
    int spriteIndex = (((((((frame % thingType->animationsFrames) * thingType->patternZ + patternZ) * 
                          thingType->patternY + patternY) * 
                          thingType->patternX + patternX) * 
                          thingType->layers + layer) * 
                          thingType->height + h) * 
                          thingType->width + w);
    
    if (spriteIndex >= 0 && spriteIndex < static_cast<int>(thingType->textureIdsVector.size())) {
        thingType->textureIdsVector[spriteIndex] = newId;
    }
}

