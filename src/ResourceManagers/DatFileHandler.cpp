#include "DatFileHandler.h"
#include "AssetsManager.h"
#include "ConfigManager.h"
#include "../Misc/Warninger.h"
#include "../Misc/definitions.h"
#include "../Misc/Timer.h"
#include "../Things/ItemType.h"
#include "../Things/Items.h"
#include "../Things/OutfitType.h"
#include "../Things/Outfits.h"
#include "../Things/EffectType.h"
#include "../Things/Effects.h"
#include "../Things/MissileType.h"
#include "../Things/Missiles.h"
#include "../Things/ThingType.h"
#include <fstream>
#include <fmt/format.h>

DatFileHandler::DatFileHandler(AssetsInfo* assetsInfo) 
    : assetsInfo(assetsInfo) {
}

void DatFileHandler::loadThingTypePatterns(std::istream& inFile, std::shared_ptr<ThingType> thingType) {
    if (!inFile.good()) {
        return;
    }

    // Read dimensions
    inFile.read(reinterpret_cast<char*>(&thingType->width), sizeof(thingType->width));
    inFile.read(reinterpret_cast<char*>(&thingType->height), sizeof(thingType->height));

    // Validate dimensions
    if (thingType->width == 0) thingType->width = 1;
    if (thingType->height == 0) thingType->height = 1;

    if (thingType->width > 1 || thingType->height > 1) {
        inFile.seekg(1, std::ios_base::cur); // Skip exact size
    }

    inFile.read(reinterpret_cast<char*>(&thingType->layers), sizeof(thingType->layers));
    inFile.read(reinterpret_cast<char*>(&thingType->patternX), sizeof(thingType->patternX));
    inFile.read(reinterpret_cast<char*>(&thingType->patternY), sizeof(thingType->patternY));
    inFile.read(reinterpret_cast<char*>(&thingType->patternZ), sizeof(thingType->patternZ));
    inFile.read(reinterpret_cast<char*>(&thingType->animationsFrames), sizeof(thingType->animationsFrames));

    // Validate pattern dimensions
    if (thingType->layers == 0) thingType->layers = 1;
    if (thingType->patternX == 0) thingType->patternX = 1;
    if (thingType->patternY == 0) thingType->patternY = 1;
    if (thingType->patternZ == 0) thingType->patternZ = 1;
    if (thingType->animationsFrames == 0) thingType->animationsFrames = 1;

    bool isAnimation = thingType->animationsFrames > 1;

    // Skip frame durations if needed
    if (isAnimation && assetsInfo->frameDurations) {
        inFile.seekg(6 + 8 * thingType->animationsFrames, std::ios_base::cur);
    }

    // Calculate number of sprites
    uint32_t numSprites = thingType->width * thingType->height * thingType->layers *
                          thingType->patternX * thingType->patternY * thingType->patternZ *
                          thingType->animationsFrames;

    // Resize vector to hold all sprites
    thingType->textureIdsVector.resize(numSprites, 0);

    // Read sprite IDs
    for (uint32_t i = 0; i < numSprites; ++i) {
        uint32_t spriteId = 0;
        if (assetsInfo->extended) {
            uint8_t bytes[4];
            inFile.read(reinterpret_cast<char*>(bytes), 4);
            if (inFile.gcount() != 4) {
                break;
            }
            spriteId = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
        } else {
            uint8_t bytes[2];
            inFile.read(reinterpret_cast<char*>(bytes), 2);
            if (inFile.gcount() != 2) {
                break;
            }
            spriteId = bytes[0] | (bytes[1] << 8);
        }
        thingType->textureIdsVector[i] = spriteId;
    }
}

void DatFileHandler::writeThingTypePatterns(std::ostream& outFile, std::shared_ptr<ThingType> thingType) {
    // Write dimensions
    outFile.write(reinterpret_cast<const char*>(&thingType->width), sizeof(thingType->width));
    outFile.write(reinterpret_cast<const char*>(&thingType->height), sizeof(thingType->height));

    // Write exact size if item is larger than 1x1
    if (thingType->width > 1 || thingType->height > 1) {
        uint8_t exactSize = 1;
        outFile.write(reinterpret_cast<const char*>(&exactSize), sizeof(exactSize));
    }

    // Validate and clamp pattern/animation values before writing
    uint8_t layers = (thingType->layers == 0) ? 1 : thingType->layers;
    uint8_t patternX = (thingType->patternX == 0) ? 1 : thingType->patternX;
    uint8_t patternY = (thingType->patternY == 0) ? 1 : thingType->patternY;
    uint8_t patternZ = (thingType->patternZ == 0) ? 1 : thingType->patternZ;
    uint8_t animationsFrames = (thingType->animationsFrames == 0) ? 1 : thingType->animationsFrames;

    // Write pattern and animation data
    outFile.write(reinterpret_cast<const char*>(&layers), sizeof(layers));
    outFile.write(reinterpret_cast<const char*>(&patternX), sizeof(patternX));
    outFile.write(reinterpret_cast<const char*>(&patternY), sizeof(patternY));
    outFile.write(reinterpret_cast<const char*>(&patternZ), sizeof(patternZ));
    outFile.write(reinterpret_cast<const char*>(&animationsFrames), sizeof(animationsFrames));

    // Write frame durations if needed
    bool isAnimation = animationsFrames > 1;
    if (isAnimation && assetsInfo->frameDurations) {
        uint8_t padding[6] = {0};
        outFile.write(reinterpret_cast<const char*>(padding), 6);
        for (uint8_t f = 0; f < animationsFrames; ++f) {
            uint64_t duration = 100; // Default 100ms per frame
            outFile.write(reinterpret_cast<const char*>(&duration), sizeof(duration));
        }
    }

    // Calculate and write sprite IDs
    uint32_t numSprites = thingType->width * thingType->height * layers *
                         patternX * patternY * patternZ * animationsFrames;

    for (uint32_t i = 0; i < numSprites && i < thingType->textureIdsVector.size(); ++i) {
        uint32_t spriteId = thingType->textureIdsVector[i];
        if (assetsInfo->extended) {
            outFile.write(reinterpret_cast<const char*>(&spriteId), sizeof(spriteId));
        } else {
            uint16_t spriteId16 = static_cast<uint16_t>(spriteId);
            outFile.write(reinterpret_cast<const char*>(&spriteId16), sizeof(spriteId16));
        }
    }
    
    // Fill remaining sprites with 0 if vector is smaller than expected
    for (uint32_t i = thingType->textureIdsVector.size(); i < numSprites; ++i) {
        if (assetsInfo->extended) {
            uint32_t zero = 0;
            outFile.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
        } else {
            uint16_t zero = 0;
            outFile.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
        }
    }
}

// loadDat and compileDat implementations are in separate files due to size:
// - DatFileHandler_Load.cpp (loadDat)
// - DatFileHandler_Compile.cpp (compileDat)

