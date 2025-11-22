// This file contains the compileDat implementation extracted from AssetsManager
// It's split into a separate file due to size

#include "DatFileHandler.h"
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

void DatFileHandler::compileDat(const std::string& outputFilePath, uint32_t signature) {
    Timer timer("Compiling .dat (OTDat)");
    
    std::string decidedPath = outputFilePath;
    if(decidedPath.empty()) {
        decidedPath = ConfigManager::getInstance()->getPathAssets() +
                      ConfigManager::getInstance()->getDatFileName();
    }
    
    try {
        std::ofstream outFile(decidedPath, std::ios::binary);
        if (!outFile.is_open()) {
            Warninger::sendErrorMsg(FUNC_NAME, "Failed to open file for writing: " + decidedPath);
            return;
        }
        
        // Write signature
        uint32_t datSignature = signature;
        if (datSignature == 0) {
            datSignature = 0x4D544154; // Default "ATMT" signature
        }
        outFile.write(reinterpret_cast<const char*>(&datSignature), sizeof(datSignature));
        
        // Get item count (items start from ID 100)
        uint16_t itemCount = static_cast<uint16_t>(Items::getItemTypesCount());
        if (itemCount < 100) {
            itemCount = 100;
        }
        outFile.write(reinterpret_cast<const char*>(&itemCount), sizeof(itemCount));
        
        // Write outfit count, effect count, missile count
        uint16_t outfitCount = static_cast<uint16_t>(Outfits::getOutfitTypesCount());
        uint16_t effectCount = static_cast<uint16_t>(Effects::getEffectTypesCount());
        uint16_t missileCount = static_cast<uint16_t>(Missiles::getMissileTypesCount());
        outFile.write(reinterpret_cast<const char*>(&outfitCount), sizeof(outfitCount));
        outFile.write(reinterpret_cast<const char*>(&effectCount), sizeof(effectCount));
        outFile.write(reinterpret_cast<const char*>(&missileCount), sizeof(missileCount));
        
        // Write items starting from ID 100
        for (uint16_t id = 100; id < itemCount; ++id) {
            auto itemType = Items::getItemType(id);
            if (!itemType) {
                uint8_t terminator = 0xFF;
                outFile.write(reinterpret_cast<const char*>(&terminator), sizeof(terminator));
                continue;
            }
            
            // Write flags
            if (itemType->hasFlag(IS_GROUND)) {
                uint8_t flag = 0x00;
                outFile.write(reinterpret_cast<const char*>(&flag), sizeof(flag));
                outFile.write(reinterpret_cast<const char*>(&itemType->speed), sizeof(itemType->speed));
            }
            if (itemType->itemCategory == GROUND_BORDER) {
                uint8_t flag = 0x01;
                outFile.write(reinterpret_cast<const char*>(&flag), sizeof(flag));
            }
            if (itemType->itemCategory == BOTTOM) {
                uint8_t flag = 0x02;
                outFile.write(reinterpret_cast<const char*>(&flag), sizeof(flag));
            }
            if (itemType->itemCategory == TOP) {
                uint8_t flag = 0x03;
                outFile.write(reinterpret_cast<const char*>(&flag), sizeof(flag));
            }
            if (itemType->hasFlag(IS_CONTAINER)) {
                uint8_t flag = 0x04;
                outFile.write(reinterpret_cast<const char*>(&flag), sizeof(flag));
            }
            if (itemType->hasFlag(STACKABLE)) {
                uint8_t flag = 0x05;
                outFile.write(reinterpret_cast<const char*>(&flag), sizeof(flag));
            }
            if (itemType->hasFlag(FORCE_USE)) {
                uint8_t flag = 0x06;
                outFile.write(reinterpret_cast<const char*>(&flag), sizeof(flag));
            }
            if (itemType->hasFlag(MULTI_USE)) {
                uint8_t flag = 0x07;
                outFile.write(reinterpret_cast<const char*>(&flag), sizeof(flag));
            }
            if (itemType->hasFlag(UNPASSABLE)) {
                uint8_t flag = 0x0C;
                outFile.write(reinterpret_cast<const char*>(&flag), sizeof(flag));
            }
            if (itemType->hasFlag(UNMOVABLE)) {
                uint8_t flag = 0x0D;
                outFile.write(reinterpret_cast<const char*>(&flag), sizeof(flag));
            }
            if (itemType->hasFlag(PICKUPABLE)) {
                uint8_t flag = 0x10;
                outFile.write(reinterpret_cast<const char*>(&flag), sizeof(flag));
            }
            
            // Write market data if name exists
            if (!itemType->name.empty()) {
                uint8_t flag = 0x21;
                outFile.write(reinterpret_cast<const char*>(&flag), sizeof(flag));
                uint16_t category = 0;
                outFile.write(reinterpret_cast<const char*>(&category), sizeof(category));
                uint16_t tradeAs = id;
                uint16_t showAs = id;
                outFile.write(reinterpret_cast<const char*>(&tradeAs), sizeof(tradeAs));
                outFile.write(reinterpret_cast<const char*>(&showAs), sizeof(showAs));
                uint16_t nameLength = static_cast<uint16_t>(itemType->name.length());
                outFile.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
                outFile.write(itemType->name.c_str(), nameLength);
                uint16_t restrictVocation = 0;
                uint16_t requiredLevel = 0;
                outFile.write(reinterpret_cast<const char*>(&restrictVocation), sizeof(restrictVocation));
                outFile.write(reinterpret_cast<const char*>(&requiredLevel), sizeof(requiredLevel));
            }
            
            // Write terminator flag
            uint8_t terminator = 0xFF;
            outFile.write(reinterpret_cast<const char*>(&terminator), sizeof(terminator));
            
            // Write texture patterns
            writeThingTypePatterns(outFile, itemType);
        }
        
        // Write outfits (starting from ID 1)
        for (uint32_t id = 1; id <= outfitCount; ++id) {
            auto outfitType = Outfits::getOutfitType(id - 1);
            if (!outfitType) {
                outfitType = std::make_shared<OutfitType>();
            }
            uint8_t lastFlag = 0xFF;
            outFile.write(reinterpret_cast<const char*>(&lastFlag), sizeof(lastFlag));
            writeThingTypePatterns(outFile, outfitType);
        }

        // Write effects (starting from ID 1)
        for (uint32_t id = 1; id <= effectCount; ++id) {
            auto effectType = Effects::getEffectType(id - 1);
            if (!effectType) {
                effectType = std::make_shared<EffectType>();
            }
            uint8_t lastFlag = 0xFF;
            outFile.write(reinterpret_cast<const char*>(&lastFlag), sizeof(lastFlag));
            writeThingTypePatterns(outFile, effectType);
        }

        // Write missiles (starting from ID 1)
        for (uint32_t id = 1; id <= missileCount; ++id) {
            auto missileType = Missiles::getMissileType(id - 1);
            if (!missileType) {
                missileType = std::make_shared<MissileType>();
            }
            uint8_t lastFlag = 0xFF;
            outFile.write(reinterpret_cast<const char*>(&lastFlag), sizeof(lastFlag));
            writeThingTypePatterns(outFile, missileType);
        }
        
        outFile.close();
        fmt::print("Compiled .dat file successfully: {}\n", decidedPath);
    } catch (const std::exception& e) {
        Warninger::sendErrorMsg(FUNC_NAME, "Failed to write .dat file '" + decidedPath + "': " + e.what());
    }
}

