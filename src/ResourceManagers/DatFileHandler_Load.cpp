// This file contains the loadDat implementation extracted from AssetsManager
// It's split into a separate file due to size

#include "DatFileHandler.h"
#include "AssetsManager.h" // For AssetsInfo definition
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

bool DatFileHandler::loadDat(const std::string& datFilePath) {
    Timer timer("Loading .dat (OTDat)");

    std::string decidedPath = datFilePath;
    if(decidedPath.empty()) {
        decidedPath = ConfigManager::getInstance()->getPathAssets() +
                      ConfigManager::getInstance()->getDatFileName();
    }

    try {
        std::ifstream inFile(decidedPath, std::ios::binary);
        if (!inFile.is_open()) {
            Warninger::sendErrorMsg(FUNC_NAME, "Failed to open file for reading: " + decidedPath);
            return false;
        }

        // Read .dat signature (4 bytes)
        uint32_t datSignature;
        inFile.read(reinterpret_cast<char*>(&datSignature), sizeof(datSignature));

        // Read item count (2 bytes)
        uint16_t itemCount;
        inFile.read(reinterpret_cast<char*>(&itemCount), sizeof(itemCount));

        // Read outfit count, effect count, and missile count (2 bytes each)
        uint16_t outfitCount, effectCount, missileCount;
        inFile.read(reinterpret_cast<char*>(&outfitCount), sizeof(outfitCount));
        inFile.read(reinterpret_cast<char*>(&effectCount), sizeof(effectCount));
        inFile.read(reinterpret_cast<char*>(&missileCount), sizeof(missileCount));
        
        fmt::print("Loading {} items, {} outfits, {} effects, {} missiles\n", 
                   itemCount, outfitCount, effectCount, missileCount);

        // Read items starting from ID 100
        uint32_t itemsToLoad = (itemCount >= 100) ? (itemCount - 100 + 1) : 0;
        
        for (uint32_t id = 0; id < itemsToLoad; ++id) {
            uint32_t actualItemId = id + 100;
            auto itemType = std::make_shared<ItemType>();
            
            if (!inFile.good()) {
                Warninger::sendErrorMsg(FUNC_NAME, "File read error before item " + std::to_string(id + 100) + ". Stopping load.");
                break;
            }

            // Read flags until we encounter 0xFF
            uint8_t flag;
            bool firstFlag = true;
            while (true) {
                inFile.read(reinterpret_cast<char*>(&flag), sizeof(flag));
                if (!inFile.good() || inFile.eof()) {
                    Warninger::sendErrorMsg(FUNC_NAME, "File read error while reading flags at item " + std::to_string(id + 100));
                    break;
                }
                
                if (flag == 0xFF) {
                    break;
                }
                
                firstFlag = false;

                switch (flag) {
                    case 0x00: // Ground
                        itemType->setFlag(IS_GROUND, true);
                        inFile.read(reinterpret_cast<char*>(&itemType->speed), sizeof(itemType->speed));
                        break;
                    case 0x01: // GroundBorder
                        itemType->itemCategory = GROUND_BORDER;
                        break;
                    case 0x02: // OnBottom
                        itemType->itemCategory = BOTTOM;
                        break;
                    case 0x03: // OnTop
                        itemType->itemCategory = TOP;
                        break;
                    case 0x04: // Container
                        itemType->setFlag(IS_CONTAINER, true);
                        break;
                    case 0x05: // Stackable
                        itemType->setFlag(STACKABLE, true);
                        break;
                    case 0x06: // ForceUse
                        itemType->setFlag(FORCE_USE, true);
                        break;
                    case 0x07: // MultiUse
                        itemType->setFlag(MULTI_USE, true);
                        break;
                    case 0x08: { // Writable
                        uint16_t maxReadWriteChars = 0;
                        inFile.read(reinterpret_cast<char *>(&maxReadWriteChars), sizeof(maxReadWriteChars));
                        break;
                    }
                    case 0x09: { // WritableOnce
                        uint16_t maxReadChars = 0;
                        inFile.read(reinterpret_cast<char *>(&maxReadChars), sizeof(maxReadChars));
                        break;
                    }
                    case 0x0C: // Unpassable
                        itemType->setFlag(UNPASSABLE, true);
                        break;
                    case 0x0D: // Unmoveable
                        itemType->setFlag(UNMOVABLE, true);
                        break;
                    case 0x0E: // BlockMissiles
                        itemType->setFlag(BLOCK_MISSILE, true);
                        break;
                    case 0x10: // Pickupable
                        itemType->setFlag(PICKUPABLE, true);
                        break;
                    case 0x15: { // HasLight
                        uint16_t lightLevel = 0;
                        uint16_t lightColor = 0;
                        inFile.read(reinterpret_cast<char *>(&lightLevel), sizeof(lightLevel));
                        inFile.read(reinterpret_cast<char *>(&lightColor), sizeof(lightColor));
                        break;
                    }
                    case 0x18: // HasOffset
                        inFile.seekg(4, std::ios_base::cur);
                        break;
                    case 0x19: // HasElevation
                        inFile.seekg(2, std::ios_base::cur);
                        break;
                    case 0x1C: { // Minimap
                        uint16_t minimapColor = 0;
                        inFile.read(reinterpret_cast<char *>(&minimapColor), sizeof(minimapColor));
                        break;
                    }
                    case 0x1D: { // LensHelp
                        uint16_t opt;
                        inFile.read(reinterpret_cast<char*>(&opt), sizeof(opt));
                        break;
                    }
                    case 0x20: // Cloth
                        inFile.seekg(2, std::ios_base::cur);
                        break;
                    case 0x21: { // Market
                        inFile.seekg(2, std::ios_base::cur);
                        uint16_t tradeAs, showAs, nameLength;
                        inFile.read(reinterpret_cast<char*>(&tradeAs), sizeof(tradeAs));
                        inFile.read(reinterpret_cast<char*>(&showAs), sizeof(showAs));
                        inFile.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
                        if (nameLength > 0 && nameLength < 256) {
                            std::vector<char> buffer(nameLength);
                            inFile.read(buffer.data(), nameLength);
                            itemType->name.assign(buffer.data(), nameLength);
                        } else {
                            inFile.seekg(nameLength, std::ios_base::cur);
                            itemType->name.clear();
                        }
                        uint16_t restrictVocation = 0;
                        uint16_t requiredLevel = 0;
                        inFile.read(reinterpret_cast<char*>(&restrictVocation), sizeof(restrictVocation));
                        inFile.read(reinterpret_cast<char*>(&requiredLevel), sizeof(requiredLevel));
                        break;
                    }
                    case 0x22: // DefaultAction
                        inFile.seekg(2, std::ios_base::cur);
                        break;
                    default:
                        // Skip unknown flags
                        break;
                }
            }

            // Read texture patterns
            if (!inFile.good() && !inFile.eof()) {
                Items::pushItemType(itemType);
                continue;
            }
            
            inFile.read(reinterpret_cast<char*>(&itemType->width), sizeof(itemType->width));
            inFile.read(reinterpret_cast<char*>(&itemType->height), sizeof(itemType->height));
            
            if (!inFile.good() && !inFile.eof()) {
                Items::pushItemType(itemType);
                continue;
            }

            if (itemType->width == 0) itemType->width = 1;
            if (itemType->height == 0) itemType->height = 1;

            if (itemType->width > 1 || itemType->height > 1) {
                inFile.seekg(1, std::ios_base::cur);
            }

            inFile.read(reinterpret_cast<char*>(&itemType->layers), sizeof(itemType->layers));
            inFile.read(reinterpret_cast<char*>(&itemType->patternX), sizeof(itemType->patternX));
            inFile.read(reinterpret_cast<char*>(&itemType->patternY), sizeof(itemType->patternY));
            inFile.read(reinterpret_cast<char*>(&itemType->patternZ), sizeof(itemType->patternZ));
            inFile.read(reinterpret_cast<char*>(&itemType->animationsFrames), sizeof(itemType->animationsFrames));
            
            if (itemType->layers == 0) itemType->layers = 1;
            if (itemType->patternX == 0) itemType->patternX = 1;
            if (itemType->patternY == 0) itemType->patternY = 1;
            if (itemType->patternZ == 0) itemType->patternZ = 1;
            if (itemType->animationsFrames == 0) itemType->animationsFrames = 1;
            
            bool isAnimation = itemType->animationsFrames > 1;

            if (isAnimation && assetsInfo->frameDurations) {
                inFile.seekg(6 + 8 * itemType->animationsFrames, std::ios_base::cur);
            }

            uint32_t numSprites = itemType->width * itemType->height * itemType->layers *
                    itemType->patternX * itemType->patternY * itemType->patternZ *
                                  itemType->animationsFrames;

            itemType->textureIdsVector.resize(numSprites, 0);
            
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
                itemType->textureIdsVector[i] = spriteId;
            }
            
            Items::pushItemType(itemType);
        }

        // Load outfits
        for (uint32_t id = 1; id <= outfitCount; ++id) {
            auto outfitType = std::make_shared<OutfitType>();
            outfitType->category = ThingCategory::OUTFIT;
            
            uint8_t flag;
            while (true) {
                inFile.read(reinterpret_cast<char*>(&flag), sizeof(flag));
                if (!inFile.good() || inFile.eof()) {
                    break;
                }
                if (flag == 0xFF) {
                    break;
                }
            }
            
            loadThingTypePatterns(inFile, outfitType);
            Outfits::pushOutfitType(outfitType);
        }

        // Load effects
        for (uint32_t id = 1; id <= effectCount; ++id) {
            auto effectType = std::make_shared<EffectType>();
            effectType->category = ThingCategory::EFFECT;
            
            uint8_t flag;
            while (true) {
                inFile.read(reinterpret_cast<char*>(&flag), sizeof(flag));
                if (!inFile.good() || inFile.eof()) {
                    break;
                }
                if (flag == 0xFF) {
                    break;
                }
            }
            
            loadThingTypePatterns(inFile, effectType);
            Effects::pushEffectType(effectType);
        }

        // Load missiles
        for (uint32_t id = 1; id <= missileCount; ++id) {
            auto missileType = std::make_shared<MissileType>();
            missileType->category = ThingCategory::MISSILE;
            
            uint8_t flag;
            while (true) {
                inFile.read(reinterpret_cast<char*>(&flag), sizeof(flag));
                if (!inFile.good() || inFile.eof()) {
                    break;
                }
                if (flag == 0xFF) {
                    break;
                }
            }
            
            loadThingTypePatterns(inFile, missileType);
            Missiles::pushMissileType(missileType);
        }

        inFile.close();
        return true;
    } catch (const std::exception& e) {
        Warninger::sendErrorMsg(FUNC_NAME, "Failed to read dat '" + decidedPath + "': " + e.what());
        return false;
    }
}

