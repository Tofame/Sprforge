#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

#include "AssetsManager.h"
#include "../Helper/SavedData.h"
#include <imgui_stdlib.h>
#include "../Misc/definitions.h"
#include "../Misc/Timer.h"
#include "../Things/Outfits.h"
#include "../Things/Effects.h"
#include "../Things/Missiles.h"
#include "../Things/ThingType.h"

AssetsManager::AssetsManager(GUIHelper* guiHelper) {
    this->guiHelper = guiHelper;

    // Setup blank texture
    auto spriteMaxSize = ConfigManager::getInstance()->getSpriteMaxSize();
    if (spriteMaxSize <= 0 || spriteMaxSize > 1024) {
        spriteMaxSize = 32; // Default fallback
        Warninger::sendWarning(FUNC_NAME, "Invalid spriteMaxSize, using default 32");
    }
    
    sf::Image image({static_cast<unsigned>(spriteMaxSize), static_cast<unsigned>(spriteMaxSize)}, sf::Color::Transparent);
    BLANK_TEXTURE = std::make_shared<sf::Texture>();
    if (!BLANK_TEXTURE->loadFromImage(image)) {
        // Try creating a minimal 1x1 texture as absolute fallback
        sf::Image fallbackImage({1, 1}, sf::Color::Transparent);
        if (!BLANK_TEXTURE->loadFromImage(fallbackImage)) {
            Warninger::sendErrorMsg(FUNC_NAME, "CRITICAL: Failed to create blank texture. Application may be unstable.");
        } else {
            Warninger::sendWarning(FUNC_NAME, "Created minimal blank texture (1x1) due to spriteMaxSize issue.");
        }
    }

    // Setup Temp Info for "New Assets" creation
    m_tempCreation_AssetsInfo.extended = SavedData::getInstance()->getDataBool("sprExtended");
    m_tempCreation_AssetsInfo.transparency = SavedData::getInstance()->getDataBool("sprTransparency");
}

AssetsManager::~AssetsManager() {
    unload();
}

std::shared_ptr<sf::Texture> AssetsManager::getTexture(int id) {
    if(!isValidTextureIndex(id)) {
        return BLANK_TEXTURE;
    }

    return textures.at(id);
}

// Helper function to read a little-endian 16-bit integer from a byte array
uint16_t readLE16(const uint8_t* data) {
    return data[0] | (data[1] << 8);
}

bool AssetsManager::loadSpr(const std::string& sprFilePath) {
    Timer timer("Loading .spr");

    std::string decidedPath = sprFilePath;
    if(decidedPath.empty()) {
        decidedPath = ConfigManager::getInstance()->getPathAssets() + "Tibia.spr";
    }

    std::ifstream file(decidedPath, std::ios::binary);
    if (!file.is_open()) {
        Warninger::sendErrorMsg(FUNC_NAME, "File not found: " + decidedPath);
        return false;
    }

    // Read and verify signature
    uint32_t signature;
    file.read(reinterpret_cast<char*>(&signature), 4);
    fmt::print("Signature of loaded spr: {}\n", signature);
    setLoadedSprSignature(signature);

    // Read sprite count (2 bytes for non-extended format)
    uint32_t spriteCount;
    if(m_assetsInfo.extended) {
        file.read(reinterpret_cast<char *>(&spriteCount), 4);
    } else {
        uint16_t tempSpriteCount;
        file.read(reinterpret_cast<char*>(&tempSpriteCount), 2);
        spriteCount = static_cast<uint32_t>(tempSpriteCount);
    }

    // Add BLANK_TEXTURE, as air (id 0)
    textures.reserve(1 + spriteCount);
    textures.push_back(BLANK_TEXTURE);

    // Read sprite offsets (4 bytes per offset)
    std::vector<uint32_t> offsets(spriteCount);
    for (uint32_t i = 0; i < spriteCount; ++i) {
        file.read(reinterpret_cast<char*>(&offsets[i]), 4);
    }

    // temp var to decide loaded sprite size
    const auto& singleSpriteSize = getSpriteDimensionsVector().at(m_assetsInfo.dimensionIndex);

    // Process each sprite
    // IMPORTANT: We must push a texture for EVERY sprite ID, even if offset is 0 or sprite is empty
    // This maintains the correct mapping: sprite ID in .dat file = texture index in our vector
    for (uint32_t spriteId = 1; spriteId <= spriteCount; ++spriteId) {
        uint32_t offset = offsets[spriteId - 1];
        std::shared_ptr<sf::Texture> texture;
        
        if (offset == 0) {
            // Sprite doesn't exist (offset 0), push blank texture to maintain index mapping
            texture = BLANK_TEXTURE;
        } else {
            file.seekg(offset, std::ios::beg);
            if (!file.good()) {
                Warninger::sendWarning(FUNC_NAME, "Failed to seek to offset for sprite " + std::to_string(spriteId) + ". Using blank texture.");
                texture = BLANK_TEXTURE;
            } else {
                file.ignore(3); // Skip unused bytes (RGB)

                // Read sprite data size
                uint16_t dataSize;
                file.read(reinterpret_cast<char*>(&dataSize), 2);
                
                if (!file.good() || dataSize == 0) {
                    // Empty sprite or read error, use blank texture
                    texture = BLANK_TEXTURE;
                } else {
                    // Read compressed sprite data
                    std::vector<uint8_t> spriteData(dataSize);
                    file.read(reinterpret_cast<char*>(spriteData.data()), dataSize);
                    
                    if (!file.good()) {
                        Warninger::sendWarning(FUNC_NAME, "Failed to read sprite data for sprite " + std::to_string(spriteId) + ". Using blank texture.");
                        texture = BLANK_TEXTURE;
                    } else {
                        // Process RLE data into RGBA pixels
                        std::vector<uint8_t> pixels(singleSpriteSize * singleSpriteSize * 4, 0); // RGBA buffer
                        size_t dataPtr = 0;
                        size_t pixelPtr = 0;

                        while (pixelPtr < singleSpriteSize * singleSpriteSize && dataPtr < spriteData.size()) {
                            // Read transparent pixels count
                            if (dataPtr + 2 > spriteData.size()) break;
                            uint16_t transparent = readLE16(&spriteData[dataPtr]);
                            dataPtr += 2;
                            pixelPtr += transparent;

                            if (pixelPtr >= singleSpriteSize * singleSpriteSize) break;

                            // Read colored pixels count
                            if (dataPtr + 2 > spriteData.size()) break;
                            uint16_t colored = readLE16(&spriteData[dataPtr]);
                            dataPtr += 2;

                            for (uint16_t i = 0; i < colored; ++i) {
                                if (pixelPtr >= singleSpriteSize * singleSpriteSize || dataPtr + 3 > spriteData.size()) break;

                                // Read RGB values
                                uint8_t r = spriteData[dataPtr++];
                                uint8_t g = spriteData[dataPtr++];
                                uint8_t b = spriteData[dataPtr++];
                                uint8_t a = m_assetsInfo.transparency ? (dataPtr < spriteData.size() ? spriteData[dataPtr++] : 255) : 255;

                                // Fill RGBA buffer
                                size_t idx = pixelPtr * 4;
                                pixels[idx] = r;
                                pixels[idx + 1] = g;
                                pixels[idx + 2] = b;
                                pixels[idx + 3] = a;

                                pixelPtr++;
                            }
                        }

                        // Create texture from pixels
                        // Create an image from pixel data, then load texture from it
                        sf::Image image(sf::Vector2u(singleSpriteSize, singleSpriteSize), pixels.data());
                        texture = std::make_shared<sf::Texture>();
                        if (texture->loadFromImage(image)) {
                            // Texture loaded successfully
                        } else {
                            Warninger::sendWarning(FUNC_NAME, "Failed to create texture for sprite " + std::to_string(spriteId) + ". Using blank texture.");
                            texture = BLANK_TEXTURE;
                        }
                    }
                }
            }
        }
        
        // ALWAYS push a texture (even if blank) to maintain correct sprite ID to texture index mapping
        textures.push_back(texture);
    }

    onGraphicsLoaded(decidedPath);
    return true;
}

void AssetsManager::compileSprFromTextures(const std::string& fileName)
{
    // temp var for an optional feature that I once used
    bool downscale64To32 = false;

    std::ofstream out(fileName, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Failed to open file for writing: " << fileName << std::endl;
        return;
    }

    // 1. Write signature
    uint32_t signature = getLoadedSprSignature();
    out.write(reinterpret_cast<const char*>(&signature), sizeof(signature));

    // 2. Write sprite count
    uint32_t spriteCount = static_cast<uint32_t>(textures.size());
    if (m_assetsInfo.extended) {
        out.write(reinterpret_cast<const char*>(&spriteCount), 4);
    } else {
        uint16_t count16 = static_cast<uint16_t>(spriteCount);
        out.write(reinterpret_cast<const char*>(&count16), 2);
    }

    // 3. Reserve space for offset table
    std::streampos offsetTableStart = out.tellp();
    std::vector<std::streampos> offsetPositions(spriteCount);
    for (uint32_t i = 0; i < spriteCount; ++i) {
        offsetPositions[i] = out.tellp();
        uint32_t placeholder = 0;
        out.write(reinterpret_cast<const char*>(&placeholder), 4);
    }

    // 4. Write each sprite
    std::vector<uint32_t> actualOffsets(spriteCount, 0);

    for (uint32_t i = 0; i < spriteCount; ++i) {
        auto texture = textures[i];
        if (!texture) continue;

        std::streampos spriteStart = out.tellp();
        actualOffsets[i] = static_cast<uint32_t>(spriteStart);

        // Write 3 unused bytes
        out.put(0).put(0).put(0);

        // Get pixel data
        const uint8_t* pixels;
        if(downscale64To32) {
            sf::RenderTexture rt({32, 32});
            sf::Sprite sprite(*texture);
            sprite.setScale({0.5f, 0.5f}); // 64 → 32 scaling

            rt.clear(sf::Color::Transparent);
            rt.draw(sprite);
            rt.display();

            sf::Image scaledImage = rt.getTexture().copyToImage();
            pixels = scaledImage.getPixelsPtr();
        } else {
            const auto& image = texture->copyToImage();
            pixels = image.getPixelsPtr();
        }

        // RLE compression
        std::vector<uint8_t> rleData;
        size_t pixelPtr = 0;
        const size_t totalPixels = 32 * 32;

        auto writeLE16 = [](std::vector<uint8_t>& data, uint16_t val) {
            data.push_back(val & 0xFF);
            data.push_back((val >> 8) & 0xFF);
        };

        auto isTransparent = [](const uint8_t* px, size_t i) {
            return (px[i * 4 + 0] == 255 &&
                    px[i * 4 + 1] == 0 &&
                    px[i * 4 + 2] == 255 && px[i * 4 + 3] == 255) || px[i * 4 + 3] == 0;
        };

        while (pixelPtr < totalPixels) {
            // Transparent pixels
            uint16_t transparentCount = 0;
            while (pixelPtr < totalPixels && isTransparent(pixels, pixelPtr)) {
                ++transparentCount;
                ++pixelPtr;
            }
            writeLE16(rleData, transparentCount);
            if (pixelPtr >= totalPixels) break;

            // Colored pixels
            uint16_t coloredCount = 0;
            size_t colorStart = pixelPtr;
            while (pixelPtr < totalPixels && !isTransparent(pixels, pixelPtr)) {
                ++coloredCount;
                ++pixelPtr;
            }

            writeLE16(rleData, coloredCount);
            for (size_t j = colorStart; j < colorStart + coloredCount; ++j) {
                const size_t idx = j * 4;
                rleData.push_back(pixels[idx]);     // R
                rleData.push_back(pixels[idx + 1]); // G
                rleData.push_back(pixels[idx + 2]); // B
                if (m_assetsInfo.transparency) {
                    rleData.push_back(pixels[idx + 3]); // A
                }
            }
        }

        // Data size
        uint16_t dataSize = static_cast<uint16_t>(rleData.size());
        out.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
        out.write(reinterpret_cast<const char*>(rleData.data()), rleData.size());
    }

    // 5. Write offset table
    out.seekp(offsetTableStart);
    for (uint32_t i = 0; i < spriteCount; ++i) {
        uint32_t offset = actualOffsets[i];
        out.write(reinterpret_cast<const char*>(&offset), sizeof(offset));
    }

    out.close();
}

bool AssetsManager::isValidTexture(std::shared_ptr<sf::Texture> texture) {
    auto spriteMaxSize = ConfigManager::getInstance()->getSpriteMaxSize();
    if(texture->getSize().x != spriteMaxSize || texture->getSize().y != spriteMaxSize) {
        return false;
    }

    return true;
}

bool AssetsManager::isValidTextureIndex(int id) {
    if (id < 0 || id >= textures.size()) {
        return false;
    }

    // Check if the pointer is not null
    return textures.at(id) != nullptr;
}

bool AssetsManager::pushTexture(std::shared_ptr<sf::Texture> texture) {
    if(!isValidTexture(texture)) {
        return false;
    }

    textures.push_back(texture);
    return true;
}

void AssetsManager::replaceTexture(int id, std::shared_ptr<sf::Texture> newTexture) {
    if(!isValidTextureIndex(id)) {
        Warninger::sendErrorMsg(FUNC_NAME, "Invalid texture id " + std::to_string(id));
        return;
    }

    textures[id] = newTexture;
}

void AssetsManager::removeTexture(int id) {
    if(!isValidTextureIndex(id)) {
        Warninger::sendErrorMsg(FUNC_NAME, "Invalid texture id " + std::to_string(id));
        return;
    }

    textures[id] = nullptr; // Set to nullptr to avoid dangling pointer

    // If last element, then reduce vector size by popping from the back
    if(id == (textures.size() - 1)) {
        textures.pop_back();
    }
}

void AssetsManager::createNewTexture() {
    getTextures().push_back(BLANK_TEXTURE);
}

ImTextureID AssetsManager::getImGuiTexture(int id) {
    if(!isValidTextureIndex(id)) {
        return (ImTextureID)(uintptr_t)BLANK_TEXTURE->getNativeHandle();
    }

    auto texture = textures.at(id);
    return (ImTextureID)(uintptr_t)texture->getNativeHandle();
}

void AssetsManager::exportTexture(const std::string& outputString, const int textureId) {
    auto texture = getTexture(textureId);
    exportTexture(outputString, *texture);
}

void AssetsManager::exportTexture(const std::string& outputString, sf::Texture texture) {
    sf::Image image = texture.copyToImage();

    if (image.saveToFile(outputString)) {
        fmt::print("Image saved successfully: {}\n", outputString);
        return;
    } else {
        Warninger::sendWarning(FUNC_NAME, "Failed to save image: " + outputString);
        return;
    }
}

void AssetsManager::compileOTDat(const std::string& outputFilePath) {
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
        
        // Write signature (use loaded signature or default)
        uint32_t datSignature = getLoadedSprSignature();
        if (datSignature == 0) {
            datSignature = 0x4D544154; // Default "ATMT" signature
        }
        outFile.write(reinterpret_cast<const char*>(&datSignature), sizeof(datSignature));
        
        // Get item count (items start from ID 100)
        uint16_t itemCount = static_cast<uint16_t>(Items::getItemTypesCount());
        // Ensure minimum count of 100 (for IDs 100-199)
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
                // Write empty item (just 0xFF terminator)
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
            
            // Write dimensions
            outFile.write(reinterpret_cast<const char*>(&itemType->width), sizeof(itemType->width));
            outFile.write(reinterpret_cast<const char*>(&itemType->height), sizeof(itemType->height));
            
            // Write exact size if item is larger than 1x1
            if (itemType->width > 1 || itemType->height > 1) {
                uint8_t exactSize = 1; // Usually 1 for items
                outFile.write(reinterpret_cast<const char*>(&exactSize), sizeof(exactSize));
            }
            
            // Validate and clamp pattern/animation values before writing
            uint8_t layers = (itemType->layers == 0) ? 1 : itemType->layers;
            uint8_t patternX = (itemType->patternX == 0) ? 1 : itemType->patternX;
            uint8_t patternY = (itemType->patternY == 0) ? 1 : itemType->patternY;
            uint8_t patternZ = (itemType->patternZ == 0) ? 1 : itemType->patternZ;
            uint8_t animationsFrames = (itemType->animationsFrames == 0) ? 1 : itemType->animationsFrames;

            // Write pattern and animation data
            outFile.write(reinterpret_cast<const char*>(&layers), sizeof(layers));
            outFile.write(reinterpret_cast<const char*>(&patternX), sizeof(patternX));
            outFile.write(reinterpret_cast<const char*>(&patternY), sizeof(patternY));
            outFile.write(reinterpret_cast<const char*>(&patternZ), sizeof(patternZ));
            outFile.write(reinterpret_cast<const char*>(&animationsFrames), sizeof(animationsFrames));
            
            // Write frame durations if needed
            bool isAnimation = animationsFrames > 1;
            if (isAnimation && m_assetsInfo.frameDurations) {
                // Write 6 bytes of padding + 8 bytes per frame
                uint8_t padding[6] = {0};
                outFile.write(reinterpret_cast<const char*>(padding), 6);
                for (uint8_t f = 0; f < animationsFrames; ++f) {
                    uint64_t duration = 100; // Default 100ms per frame
                    outFile.write(reinterpret_cast<const char*>(&duration), sizeof(duration));
                }
            }

            // Calculate and write sprite IDs in correct order
            uint32_t numSprites = itemType->width * itemType->height * layers *
                                 patternX * patternY * patternZ *
                                 animationsFrames;
            
            // Write sprites in the same order as they're stored in textureIdsVector
            for (uint32_t i = 0; i < numSprites && i < itemType->textureIdsVector.size(); ++i) {
                uint32_t spriteId = itemType->textureIdsVector[i];
                if (m_assetsInfo.extended) {
                    outFile.write(reinterpret_cast<const char*>(&spriteId), sizeof(spriteId));
                } else {
                    uint16_t spriteId16 = static_cast<uint16_t>(spriteId);
                    outFile.write(reinterpret_cast<const char*>(&spriteId16), sizeof(spriteId16));
                }
            }
            
            // Fill remaining sprites with 0 if vector is smaller than expected
            for (uint32_t i = itemType->textureIdsVector.size(); i < numSprites; ++i) {
                if (m_assetsInfo.extended) {
                    uint32_t zero = 0;
                    outFile.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
                } else {
                    uint16_t zero = 0;
                    outFile.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
                }
            }
        }
        
        // Write outfits (starting from ID 1)
        for (uint32_t id = 1; id <= outfitCount; ++id) {
            auto outfitType = Outfits::getOutfitType(id - 1); // 0-based index
            if (!outfitType) {
                outfitType = std::make_shared<OutfitType>();
            }
            // Write properties (for outfits, usually just 0xFF if no properties)
            uint8_t lastFlag = 0xFF;
            outFile.write(reinterpret_cast<const char*>(&lastFlag), sizeof(lastFlag));
            // Write texture patterns
            writeThingTypePatterns(outFile, outfitType);
        }

        // Write effects (starting from ID 1)
        for (uint32_t id = 1; id <= effectCount; ++id) {
            auto effectType = Effects::getEffectType(id - 1); // 0-based index
            if (!effectType) {
                effectType = std::make_shared<EffectType>();
            }
            // Write properties (for effects, usually just 0xFF if no properties)
            // TODO: Handle TOP_EFFECT flag (0x23) if effectType->topEffect is true
            uint8_t lastFlag = 0xFF;
            outFile.write(reinterpret_cast<const char*>(&lastFlag), sizeof(lastFlag));
            // Write texture patterns
            writeThingTypePatterns(outFile, effectType);
        }

        // Write missiles (starting from ID 1)
        for (uint32_t id = 1; id <= missileCount; ++id) {
            auto missileType = Missiles::getMissileType(id - 1); // 0-based index
            if (!missileType) {
                missileType = std::make_shared<MissileType>();
            }
            // Write properties (for missiles, usually just 0xFF if no properties)
            uint8_t lastFlag = 0xFF;
            outFile.write(reinterpret_cast<const char*>(&lastFlag), sizeof(lastFlag));
            // Write texture patterns
            writeThingTypePatterns(outFile, missileType);
        }
        
        outFile.close();
        fmt::print("Compiled .dat file successfully: {}\n", decidedPath);
    } catch (const std::exception& e) {
        Warninger::sendErrorMsg(FUNC_NAME, "Failed to write .dat file '" + decidedPath + "': " + e.what());
    }
}

void AssetsManager::loadOTDat(const std::string &datFilePath) {
    // TO-DO protocol version should be detected, as currently OTDat is done for 8.6.
    // Higher protocol versions had offset of all flags +1 because there was a flag inserted in the middle xD

    Timer timer("Loading .dat (OTDat)");

    std::string decidedPath = datFilePath;
    if(decidedPath.empty()) {
        decidedPath = ConfigManager::getInstance()->getPathAssets() +
                      ConfigManager::getInstance()->getDatFileName();
    };

    try {
        // Open the file in binary mode for reading
        std::ifstream inFile(decidedPath, std::ios::binary);
        if (!inFile.is_open()) {
            Warninger::sendErrorMsg(FUNC_NAME, "Failed to open file for reading: " + decidedPath);
            return;
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
        // ObjectBuilder loops from minID to maxID (inclusive): for (id = minID; id <= maxID; id++)
        // For items: minID = 100, maxID = itemCount
        // So we load items with IDs 100 through itemCount (inclusive)
        // That's (itemCount - 100 + 1) items
        uint32_t itemsToLoad = (itemCount >= 100) ? (itemCount - 100 + 1) : 0;
        fmt::print("Loading {} items (itemCount={}, IDs 100-{} inclusive)\n", itemsToLoad, itemCount, itemCount);
        
        for (uint32_t id = 0; id < itemsToLoad; ++id) {
            uint32_t actualItemId = id + 100; // Actual item ID in file
            auto itemType = std::make_shared<ItemType>();
            
            // Check if file read is still valid
            if (!inFile.good()) {
                Warninger::sendErrorMsg(FUNC_NAME, "File read error before item " + std::to_string(id + 100) + ". Stopping load.");
                break;
            }

            // Read flags until we encounter 0xFF (ItemFlag.LastFlag)
            // ObjectBuilder always reads flags and texture patterns for every item ID, even if item doesn't exist
            uint8_t flag;
            bool firstFlag = true;
            while (true) {
                inFile.read(reinterpret_cast<char*>(&flag), sizeof(flag));
                if (!inFile.good() || inFile.eof()) {
                    Warninger::sendErrorMsg(FUNC_NAME, "File read error while reading flags at item " + std::to_string(id + 100));
                    break;
                }
                
                if (flag == 0xFF) {
                    // LastFlag - if this is the first flag, item doesn't exist, but we still read texture patterns
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
                        //itemType->readable = true;
                        uint16_t maxReadWriteChars = 0;
                        inFile.read(reinterpret_cast<char *>(&maxReadWriteChars), sizeof(maxReadWriteChars));
                        break;
                    }
                    case 0x09: { // WritableOnce
                        //itemType->readable = true;
                        uint16_t maxReadChars = 0;
                        inFile.read(reinterpret_cast<char *>(&maxReadChars), sizeof(maxReadChars));
                        break;
                    }
                    case 0x0A: // FluidContainer
                        //itemType->type = ITEM_TYPE_FLUID;
                        break;
                    case 0x0B: // Fluid
                        //itemType->type = ITEM_TYPE_SPLASH;
                        break;
                    case 0x0C: // Unpassable
                        itemType->setFlag(UNPASSABLE, true);
                        break;
                    case 0x0D: // Unmoveable
                        itemType->setFlag(UNMOVABLE, true);
                        break;
                    case 0x0E: // BlockMissiles
                        itemType->setFlag(BLOCK_MISSILE, true);
                        break;
                    case 0x0F: // BlockPathfinder
                        // Not implemented in current flag system
                        break;
//                    case 0x10: // NoMoveAnimation
//                        // Not implemented
//                        break;
                    case 0x10: // Pickupable
                        itemType->setFlag(PICKUPABLE, true);
                        break;
                    case 0x11: // Hangable
                        //itemType->hangable = true;
                        break;
                    case 0x12: // Horizontal
                        //itemType->hookEast = true;
                        break;
                    case 0x13: // Vertical
                        //itemType->hookSouth = true;
                        break;
                    case 0x14: // Rotatable
                        //itemType->rotatable = true;
                        break;
                    case 0x15: { // HasLight
                        uint16_t lightLevel = 0;
                        uint16_t lightColor = 0;
                        inFile.read(reinterpret_cast<char *>(&lightLevel), sizeof(lightLevel));
                        inFile.read(reinterpret_cast<char *>(&lightColor), sizeof(lightColor));
                        break;
                    }
                    case 0x16: // DontHide
                        break;
                    case 0x17: // Translucent
                        break;
                    case 0x18: // HasOffset
                        inFile.seekg(4, std::ios_base::cur); // Skip offsetX and offsetY
                        break;
                    case 0x19: // HasElevation
                        //itemType->hasElevation = true;
                        inFile.seekg(2, std::ios_base::cur); // Skip height
                        break;
                    case 0x1A: // Lying
                        break;
                    case 0x1B: // AnimateAlways
                        break;
                    case 0x1C: { // Minimap
                        uint16_t minimapColor = 0;
                        inFile.read(reinterpret_cast<char *>(&minimapColor), sizeof(minimapColor));
                        break;
                    }
                    case 0x1D: // LensHelp
                        uint16_t opt;
                        inFile.read(reinterpret_cast<char*>(&opt), sizeof(opt));
                        if (opt == 1112) {
                            //itemType->readable = true;
                        }
                        break;
                    case 0x1E: // FullGround
                        //itemType->fullGround = true;
                        break;
                    case 0x1F: // IgnoreLook
                        //itemType->ignoreLook = true;
                        break;
                    case 0x20: // Cloth
                        inFile.seekg(2, std::ios_base::cur); // Skip cloth value
                        break;
                    case 0x21: { // Market
                        inFile.seekg(2, std::ios_base::cur); // Skip category
                        uint16_t tradeAs, showAs, nameLength;

                        inFile.read(reinterpret_cast<char*>(&tradeAs), sizeof(tradeAs));
                        inFile.read(reinterpret_cast<char*>(&showAs), sizeof(showAs));
                        inFile.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));

                        // Validate nameLength before reading
                        if (nameLength > 0 && nameLength < 256) { // Reasonable upper limit
                            std::vector<char> buffer(nameLength);
                            inFile.read(buffer.data(), nameLength);
                            itemType->name.assign(buffer.data(), nameLength);
                        } else {
                            // Skip corrupted name
                            inFile.seekg(nameLength, std::ios_base::cur);
                            itemType->name.clear();
                        }

                        // Read remaining fields
                        uint16_t restrictVocation = 0;
                        uint16_t requiredLevel = 0;
                        inFile.read(reinterpret_cast<char*>(&restrictVocation), sizeof(restrictVocation));
                        inFile.read(reinterpret_cast<char*>(&requiredLevel), sizeof(requiredLevel));
                        break;
                    }
                    case 0x22: // DefaultAction
                        inFile.seekg(2, std::ios_base::cur); // Skip action
                        break;
                    case 0x23: // Wrappable
                    case 0x24: // Unwrappable
                    case 0x25: // TopEffect
                    case 0x26: // Usable
                        break;
                    case 0xFE: // 254
                        break;
                    default:
                        Warninger::sendErrorMsg(FUNC_NAME, "Unknown flag 0x" + std::to_string(flag) + " at id " + std::to_string(id));
                        //delete itemType;
                        //inFile.close();
                        //return;
                        break;
                }
            }

            // Read texture patterns data (always read, even if item doesn't exist)
            // ObjectBuilder always reads texture patterns for every item ID, even if item doesn't exist
            // Check if we can read the dimensions
            if (!inFile.good() && !inFile.eof()) {
                Warninger::sendErrorMsg(FUNC_NAME, "File read error before reading dimensions at item " + std::to_string(actualItemId) + ". Pushing empty item.");
                // Still push empty item to maintain count (ObjectBuilder always pushes)
                Items::pushItemType(itemType);
                continue;
            }
            
            inFile.read(reinterpret_cast<char*>(&itemType->width), sizeof(itemType->width));
            inFile.read(reinterpret_cast<char*>(&itemType->height), sizeof(itemType->height));
            
            if (!inFile.good() && !inFile.eof()) {
                Warninger::sendErrorMsg(FUNC_NAME, "File read error after reading dimensions at item " + std::to_string(actualItemId) + ". Pushing partial item.");
                // Still push item to maintain count
                Items::pushItemType(itemType);
                continue;
            }

            // Validate dimensions - ensure they're at least 1
            if (itemType->width == 0) itemType->width = 1;
            if (itemType->height == 0) itemType->height = 1;

            if (itemType->width > 1 || itemType->height > 1) {
                inFile.seekg(1, std::ios_base::cur); // Skip exact size
            }

            inFile.read(reinterpret_cast<char*>(&itemType->layers), sizeof(itemType->layers));
            inFile.read(reinterpret_cast<char*>(&itemType->patternX), sizeof(itemType->patternX));
            inFile.read(reinterpret_cast<char*>(&itemType->patternY), sizeof(itemType->patternY));
            inFile.read(reinterpret_cast<char*>(&itemType->patternZ), sizeof(itemType->patternZ));
            inFile.read(reinterpret_cast<char*>(&itemType->animationsFrames), sizeof(itemType->animationsFrames));
            
            // Validate pattern dimensions - ensure they're at least 1 (for 8.6 protocol, 0 means not used, but we need at least 1 for calculations)
            // Note: Some items may legitimately have 0 in the file, but we need at least 1 for our indexing calculations
            if (itemType->layers == 0) itemType->layers = 1;
            if (itemType->patternX == 0) itemType->patternX = 1;
            if (itemType->patternY == 0) itemType->patternY = 1;
            if (itemType->patternZ == 0) itemType->patternZ = 1;
            if (itemType->animationsFrames == 0) itemType->animationsFrames = 1;
            
            // Debug: Log pattern dimensions for first few items to verify reading
            if (actualItemId <= 105) {
                fmt::print("Item {}: width={}, height={}, layers={}, patternX={}, patternY={}, patternZ={}, frames={}\n",
                          actualItemId, itemType->width, itemType->height, itemType->layers,
                          itemType->patternX, itemType->patternY, itemType->patternZ, itemType->animationsFrames);
            }
            
            bool isAnimation = itemType->animationsFrames > 1;

            // Skip frame durations if needed
            if (isAnimation && m_assetsInfo.frameDurations) {
                inFile.seekg(6 + 8 * itemType->animationsFrames, std::ios_base::cur);
            }

            // Calculate number of sprites (must match getCalcIndexesCount formula)
            uint32_t numSprites = itemType->width * itemType->height * itemType->layers *
                    itemType->patternX * itemType->patternY * itemType->patternZ *
                                  itemType->animationsFrames;

            // Resize vector to hold all sprites
            itemType->textureIdsVector.resize(numSprites, 0);
            
            // Read sprite IDs in the order they're stored in the file
            // Order in file matches getSpriteIndex formula: frame -> patternZ -> patternY -> patternX -> layers -> height -> width
            // ObjectBuilder uses LITTLE_ENDIAN, so we need to read bytes correctly
            for (uint32_t i = 0; i < numSprites; ++i) {
                uint32_t spriteId = 0;
                if (m_assetsInfo.extended) {
                    // Read 32-bit little-endian
                    uint8_t bytes[4];
                    inFile.read(reinterpret_cast<char*>(bytes), 4);
                    if (inFile.gcount() != 4) {
                        Warninger::sendErrorMsg(FUNC_NAME, "Failed to read sprite ID at item " + std::to_string(actualItemId) + ", sprite " + std::to_string(i));
                        break; // Stop reading sprites for this item
                    }
                    spriteId = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
                } else {
                    // Read 16-bit little-endian
                    uint8_t bytes[2];
                    inFile.read(reinterpret_cast<char*>(bytes), 2);
                    if (inFile.gcount() != 2) {
                        Warninger::sendErrorMsg(FUNC_NAME, "Failed to read sprite ID at item " + std::to_string(actualItemId) + ", sprite " + std::to_string(i));
                        break; // Stop reading sprites for this item
                    }
                    spriteId = bytes[0] | (bytes[1] << 8);
                }

                itemType->textureIdsVector[i] = spriteId;
            }
            
            // Check if we successfully read all sprites
            if (inFile.fail() && !inFile.eof()) {
                Warninger::sendErrorMsg(FUNC_NAME, "File read error at item " + std::to_string(actualItemId) + ". Pushing partial item.");
                // Still push the item to maintain count (ObjectBuilder always pushes, even on error)
            }

            // Store the item (ALWAYS push, even if empty/incomplete, to maintain correct count)
            // ObjectBuilder always pushes an item for every ID from minID to maxID
            Items::pushItemType(itemType);
        }

        // Load outfits (starting from ID 1)
        // ObjectBuilder loops from minID to maxID (inclusive): for (id = minID; id <= maxID; id++)
        // For outfits: minID = 1, maxID = outfitCount
        // So we load outfits with IDs 1 through outfitCount (inclusive)
        for (uint32_t id = 1; id <= outfitCount; ++id) {
            auto outfitType = std::make_shared<OutfitType>();
            outfitType->category = ThingCategory::OUTFIT;
            
            // Read properties (for outfits/effects/missiles, this is usually just 0xFF if no properties)
            // But we still need to read it to advance the file pointer
            uint8_t flag;
            while (true) {
                inFile.read(reinterpret_cast<char*>(&flag), sizeof(flag));
                if (!inFile.good() || inFile.eof()) {
                    Warninger::sendErrorMsg(FUNC_NAME, "File read error while reading outfit properties at ID " + std::to_string(id));
                    break;
                }
                if (flag == 0xFF) { // LAST_FLAG
                    break;
                }
                // For now, we don't handle outfit-specific properties, just skip any data
                // In the future, we might need to handle outfit properties here
            }
            
            // Read texture patterns
            loadThingTypePatterns(inFile, outfitType);
            Outfits::pushOutfitType(outfitType);
        }

        // Load effects (starting from ID 1)
        for (uint32_t id = 1; id <= effectCount; ++id) {
            auto effectType = std::make_shared<EffectType>();
            effectType->category = ThingCategory::EFFECT;
            
            // Read properties
            uint8_t flag;
            while (true) {
                inFile.read(reinterpret_cast<char*>(&flag), sizeof(flag));
                if (!inFile.good() || inFile.eof()) {
                    Warninger::sendErrorMsg(FUNC_NAME, "File read error while reading effect properties at ID " + std::to_string(id));
                    break;
                }
                if (flag == 0xFF) { // LAST_FLAG
                    break;
                }
                // Handle effect-specific properties if needed
                // For now, effects might have TOP_EFFECT flag (0x23 in MetadataFlags6)
                if (flag == 0x23) { // TOP_EFFECT
                    // effectType->topEffect = true; // If we add this property later
                }
            }
            
            // Read texture patterns
            loadThingTypePatterns(inFile, effectType);
            Effects::pushEffectType(effectType);
        }

        // Load missiles (starting from ID 1)
        for (uint32_t id = 1; id <= missileCount; ++id) {
            auto missileType = std::make_shared<MissileType>();
            missileType->category = ThingCategory::MISSILE;
            
            // Read properties
            uint8_t flag;
            while (true) {
                inFile.read(reinterpret_cast<char*>(&flag), sizeof(flag));
                if (!inFile.good() || inFile.eof()) {
                    Warninger::sendErrorMsg(FUNC_NAME, "File read error while reading missile properties at ID " + std::to_string(id));
                    break;
                }
                if (flag == 0xFF) { // LAST_FLAG
                    break;
                }
                // Missiles typically don't have properties, but we handle it just in case
            }
            
            // Read texture patterns
            loadThingTypePatterns(inFile, missileType);
            Missiles::pushMissileType(missileType);
        }

        inFile.close();
        onDatLoaded(decidedPath);
    } catch (const std::exception& e) {
        Warninger::sendErrorMsg(FUNC_NAME, "Failed to read dat '" + decidedPath + "': " + e.what());
    }
}

bool AssetsManager::hasUnsavedChanges(ASSET_CATEGORY fromCategory) const {
    switch(fromCategory) {
        case CATEGORY_ITEMS:
            return unsavedItemChanges;
        case CATEGORY_ITEMS_ITEMTYPE:
            return unsavedItemTypeChange;
        case CATEGORY_SPRITES:
            return unsavedSpriteChanges;
        case CATEGORY_MAIN_ONES:
            return (unsavedSpriteChanges || unsavedItemChanges);
        default:
            return false;
    }
}

void AssetsManager::setUnsavedChanges(ASSET_CATEGORY fromCategory, bool value) {
    switch(fromCategory) {
        case CATEGORY_ITEMS:
            unsavedItemChanges = value;
            break;
        case CATEGORY_ITEMS_ITEMTYPE:
            unsavedItemTypeChange = value;
            break;
        case CATEGORY_SPRITES:
            unsavedSpriteChanges = value;
            break;
        case CATEGORY_MAIN_ONES:
            unsavedSpriteChanges = value;
            unsavedItemChanges = value;
            break;
        default:
            return;
    }
}

uint32_t AssetsManager::getTextureIdFromThingType(const std::shared_ptr<ThingType>& thingType, int w, int h, int a, 
                                      int layer, int patternX, int patternY, int patternZ) {
    // Validate input parameters
    if (!thingType || thingType->width == 0 || thingType->height == 0 || thingType->layers == 0 || 
        thingType->patternX == 0 || thingType->patternY == 0 || thingType->patternZ == 0 || thingType->animationsFrames == 0) {
        return 0;
    }
    
    if (w < 0 || w >= thingType->width || h < 0 || h >= thingType->height ||
        layer < 0 || layer >= thingType->layers ||
        patternX < 0 || patternX >= thingType->patternX ||
        patternY < 0 || patternY >= thingType->patternY ||
        patternZ < 0 || patternZ >= thingType->patternZ) {
        return 0;
    }
    
    int frame = a - 1;
    if (frame < 0) frame = 0;
    if (frame >= thingType->animationsFrames) frame = frame % thingType->animationsFrames;
    
    int spriteIndex = (((((((frame % thingType->animationsFrames) * thingType->patternZ + patternZ) * 
                          thingType->patternY + patternY) * 
                          thingType->patternX + patternX) * 
                          thingType->layers + layer) * 
                          thingType->height + h) * 
                          thingType->width + w);
    
    if (spriteIndex < 0 || spriteIndex >= static_cast<int>(thingType->textureIdsVector.size())) {
        return 0;
    }
    
    return thingType->textureIdsVector[spriteIndex];
}

void AssetsManager::setTextureIdFromThingType(std::shared_ptr<ThingType> thingType, int w, int h, int a, int newId,
                                   int layer, int patternX, int patternY, int patternZ) {
    if (!thingType || thingType->width == 0 || thingType->height == 0 || thingType->layers == 0 || 
        thingType->patternX == 0 || thingType->patternY == 0 || thingType->patternZ == 0 || thingType->animationsFrames == 0) {
        return;
    }
    
    if (w < 0 || w >= thingType->width || h < 0 || h >= thingType->height ||
        layer < 0 || layer >= thingType->layers ||
        patternX < 0 || patternX >= thingType->patternX ||
        patternY < 0 || patternY >= thingType->patternY ||
        patternZ < 0 || patternZ >= thingType->patternZ) {
        return;
    }
    
    int frame = a - 1;
    if (frame < 0) frame = 0;
    if (frame >= thingType->animationsFrames) frame = frame % thingType->animationsFrames;
    
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

std::vector<std::shared_ptr<sf::Texture>>& AssetsManager::getPreviewTexturesVector(ThingCategory category) {
    switch(category) {
        case ThingCategory::ITEM:
            return previewTexturesItems;
        case ThingCategory::OUTFIT:
            return previewTexturesOutfits;
        case ThingCategory::EFFECT:
            return previewTexturesEffects;
        case ThingCategory::MISSILE:
            return previewTexturesMissiles;
        default:
            return previewTexturesItems;
    }
}

std::shared_ptr<sf::Texture> AssetsManager::getPreviewTexture(int thingTypeId, ThingCategory category) {
    auto& previewTextures = getPreviewTexturesVector(category);
    if(thingTypeId < previewTextures.size() && previewTextures.at(thingTypeId)) {
        return previewTextures[thingTypeId];
    }
    return BLANK_TEXTURE;
}

void AssetsManager::replacePreviewTexture(int thingTypeId, std::shared_ptr<sf::Texture> texture, ThingCategory category) {
    auto& previewTextures = getPreviewTexturesVector(category);
    if(thingTypeId < 0) {
        return;
    }
    if(thingTypeId >= previewTextures.size()) {
        previewTextures.resize(thingTypeId + 1);
    }
    previewTextures[thingTypeId] = texture;
}

void AssetsManager::createPreviewTexture(int id, ThingCategory category) {
    std::shared_ptr<ThingType> thingType;
    bool isValid = false;
    
    switch(category) {
        case ThingCategory::ITEM:
            isValid = Items::isValidItemTypeIndex(id);
            if (isValid) thingType = Items::getItemType(id);
            break;
        case ThingCategory::OUTFIT:
            isValid = Outfits::isValidOutfitTypeIndex(id);
            if (isValid) thingType = Outfits::getOutfitType(id);
            break;
        case ThingCategory::EFFECT:
            isValid = Effects::isValidEffectTypeIndex(id);
            if (isValid) thingType = Effects::getEffectType(id);
            break;
        case ThingCategory::MISSILE:
            isValid = Missiles::isValidMissileTypeIndex(id);
            if (isValid) thingType = Missiles::getMissileType(id);
            break;
    }
    
    auto& previewTextures = getPreviewTexturesVector(category);
    
    if (!isValid || !thingType || thingType->width == 0 || thingType->height == 0) {
        if (id >= previewTextures.size()) {
            previewTextures.resize(id + 1);
        }
        replacePreviewTexture(id, BLANK_TEXTURE, category);
        return;
    }

    auto thingPreviewTexture = getThingSpriteSheet(id, 1, category);
    
    if (thingPreviewTexture.getSize().x == 0 || thingPreviewTexture.getSize().y == 0) {
        if (id >= previewTextures.size()) {
            previewTextures.resize(id + 1);
        }
        replacePreviewTexture(id, BLANK_TEXTURE, category);
        return;
    }

    auto finalSprite = sf::Sprite(thingPreviewTexture);
    auto SizeOfButtonSprite = ConfigManager::getInstance()->getSpriteButtonSize();
    finalSprite.setScale({
         static_cast<float>(SizeOfButtonSprite.x) / thingPreviewTexture.getSize().x,
         static_cast<float>(SizeOfButtonSprite.y) / thingPreviewTexture.getSize().y
     });

    sf::RenderTexture scaledRender (sf::Vector2u(SizeOfButtonSprite.x, SizeOfButtonSprite.y));
    if (scaledRender.getSize().x == 0 || scaledRender.getSize().y == 0) {
        if (id >= previewTextures.size()) {
            previewTextures.resize(id + 1);
        }
        replacePreviewTexture(id, BLANK_TEXTURE, category);
        return;
    }
    
    scaledRender.clear(sf::Color::Transparent);
    scaledRender.draw(finalSprite);
    scaledRender.display();

    auto texture = std::make_shared<sf::Texture>(scaledRender.getTexture());
    if (id >= previewTextures.size()) {
        previewTextures.resize(id + 1);
    }
    replacePreviewTexture(id, texture, category);
}

void AssetsManager::createPreviewTexturesForPage(int pageFirstThingType, int pageLastThingType, ThingCategory category) {
    for (int id = pageFirstThingType; id <= pageLastThingType; ++id) {
        bool isValid = false;
        std::shared_ptr<ThingType> thingType;
        
        switch(category) {
            case ThingCategory::ITEM:
                isValid = Items::isValidItemTypeIndex(id);
                if (isValid) thingType = Items::getItemType(id);
                break;
            case ThingCategory::OUTFIT:
                isValid = Outfits::isValidOutfitTypeIndex(id);
                if (isValid) thingType = Outfits::getOutfitType(id);
                break;
            case ThingCategory::EFFECT:
                isValid = Effects::isValidEffectTypeIndex(id);
                if (isValid) thingType = Effects::getEffectType(id);
                break;
            case ThingCategory::MISSILE:
                isValid = Missiles::isValidMissileTypeIndex(id);
                if (isValid) thingType = Missiles::getMissileType(id);
                break;
        }
        
        auto& previewTextures = getPreviewTexturesVector(category);
        
        if (!isValid || !thingType || thingType->width == 0 || thingType->height == 0) {
            if (id >= previewTextures.size()) {
                previewTextures.resize(id + 1);
            }
            replacePreviewTexture(id, BLANK_TEXTURE, category);
            continue;
        }
        
        try {
            createPreviewTexture(id, category);
        } catch (const std::exception& e) {
            Warninger::sendWarning(FUNC_NAME, "Exception creating preview for thing " + std::to_string(id) + ": " + e.what());
            if (id >= previewTextures.size()) {
                previewTextures.resize(id + 1);
            }
            replacePreviewTexture(id, BLANK_TEXTURE, category);
        }
    }
}

void AssetsManager::clearPreviewTextures() {
    previewTexturesItems.clear();
    previewTexturesOutfits.clear();
    previewTexturesEffects.clear();
    previewTexturesMissiles.clear();
    previewTexturesItems.shrink_to_fit();
    previewTexturesOutfits.shrink_to_fit();
    previewTexturesEffects.shrink_to_fit();
    previewTexturesMissiles.shrink_to_fit();
}

sf::Texture AssetsManager::getThingSpriteSheet(int thingTypeId, int animations, ThingCategory category) {
    std::shared_ptr<ThingType> thingType;
    bool isValid = false;
    
    switch(category) {
        case ThingCategory::ITEM:
            isValid = Items::isValidItemTypeIndex(thingTypeId);
            if (isValid) thingType = Items::getItemType(thingTypeId);
            break;
        case ThingCategory::OUTFIT:
            isValid = Outfits::isValidOutfitTypeIndex(thingTypeId);
            if (isValid) thingType = Outfits::getOutfitType(thingTypeId);
            break;
        case ThingCategory::EFFECT:
            isValid = Effects::isValidEffectTypeIndex(thingTypeId);
            if (isValid) thingType = Effects::getEffectType(thingTypeId);
            break;
        case ThingCategory::MISSILE:
            isValid = Missiles::isValidMissileTypeIndex(thingTypeId);
            if (isValid) thingType = Missiles::getMissileType(thingTypeId);
            break;
    }
    
    if (!isValid || !thingType) {
        Warninger::sendWarning(FUNC_NAME, "Invalid thingType id: " + std::to_string(thingTypeId));
        return sf::Texture(*BLANK_TEXTURE);
    }

    if (thingType->width == 0 || thingType->height == 0 || thingType->width > 255 || thingType->height > 255) {
        Warninger::sendWarning(FUNC_NAME, "Invalid dimensions for ThingType (" + std::to_string(thingTypeId) + 
                               "): width=" + std::to_string(thingType->width) + ", height=" + std::to_string(thingType->height));
        return sf::Texture(*BLANK_TEXTURE);
    }

    if (animations <= 0) {
        animations = 1;
    }

    auto spriteMaxSize = ConfigManager::getInstance()->getSpriteMaxSize();
    int singleAnimationFrameSize = thingType->height * spriteMaxSize;

    sf::Vector2u size{
            static_cast<unsigned>(thingType->width * spriteMaxSize),
            static_cast<unsigned>(singleAnimationFrameSize * animations)
    };
    
    if (size.x == 0 || size.y == 0 || size.x > 8192 || size.y > 8192) {
        Warninger::sendWarning(FUNC_NAME, "Invalid texture size for ThingType (" + std::to_string(thingTypeId) + 
                               "): " + std::to_string(size.x) + "x" + std::to_string(size.y));
        return sf::Texture(*BLANK_TEXTURE);
    }
    
    sf::RenderTexture render(size);
    if (render.getSize().x == 0 || render.getSize().y == 0) {
        Warninger::sendWarning(FUNC_NAME, "Failed to create RenderTexture for ThingType (" + std::to_string(thingTypeId) + ")");
        return sf::Texture(*BLANK_TEXTURE);
    }

    render.clear(sf::Color::Transparent);

    for (int a = 1; a <= animations; a++) {
        for (int l = 0; l < thingType->layers; l++) {
            for (int w = 0; w < thingType->width; w++) {
                for (int h = 0; h < thingType->height; h++) {
                    auto texture = getTexture(getTextureIdFromThingType(thingType, w, h, a, l, 0, 0, 0));
                    if (texture) {
                        sf::Sprite sprite(*texture);
                        sprite.setPosition(sf::Vector2f(
                                static_cast<float>((thingType->width - w - 1) * spriteMaxSize),
                                static_cast<float>((thingType->height - h - 1) * spriteMaxSize + ((a - 1) * singleAnimationFrameSize))
                        ));
                        render.draw(sprite);
                    }
                }
            }
        }
    }
    render.display();

    sf::Texture spriteSheetTexture = sf::Texture(render.getTexture());
    return spriteSheetTexture;
}

void AssetsManager::drawAssetsManagerControls() {
    auto newAssetsIcon = getGuiHelper()->getImGuiTexture("icon_newAssets");
    if (ImGui::ImageButton("##ControlButton_NewAssets", newAssetsIcon, {16,16})) {
        ImGui::OpenPopup("Asset Creation Details");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Create New Assets");
    }
    if (ImGui::BeginPopupModal("Asset Creation Details", nullptr)) {
        doPopupNewAssetFiles();
    }

    ImGui::SameLine();
    auto openAssetsIcon = getGuiHelper()->getImGuiTexture("icon_openAssets");
    if (ImGui::ImageButton("##ControlButton_OpenAssets", openAssetsIcon, {16,16})) {
        ImGui::OpenPopup("Asset File Details");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Load Assets");
    }
    if (ImGui::BeginPopupModal("Asset File Details", nullptr)) {
        doPopupAssetFileOpen();
    }

    // Add the "Compile .spr" button
    ImGui::SameLine();
    auto compileAssetsIcon = getGuiHelper()->getImGuiTexture("icon_compileAssets");
    auto colorsCount = Tools::pushImGuiGray(!isCompilable());
    if(colorsCount > 0) {
        ImGui::BeginDisabled();
    }
    if (ImGui::ImageButton("##ControlButton_CompileAssets", compileAssetsIcon, {16,16})) {
        compile();
    }
    if(colorsCount > 0) {
        ImGui::EndDisabled();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Compile");
    }
    ImGui::PopStyleColor(colorsCount);

    ImGui::SameLine();
    auto compileAssetsAsIcon = getGuiHelper()->getImGuiTexture("icon_compileAssetsAs");
    if (ImGui::ImageButton("##ControlButton_CompileAssetsAs", compileAssetsAsIcon, {16,16})) {
        if(m_assetsInfo.outputPath.empty()) {
            m_assetsInfo.outputPath = SavedData::getInstance()->getDataString("tempLoadedGraphicFilePath").empty() ?
                std::filesystem::current_path().string() + "/data/things/"
                : Tools::cleanPathIntoFolderPath(SavedData::getInstance()->getDataString("tempLoadedGraphicFilePath"));
        }
        ImGui::OpenPopup("Compile Assets Files");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Compile As");
    }
    if (ImGui::BeginPopupModal("Compile Assets Files", nullptr)) {
        doPopupAssetsCompileAs();
    }

    ImGui::Separator();
}

void AssetsManager::onGraphicsLoaded(const std::string& loadedPath) {
    fmt::print("Finished loading graphics from {}\nTotal: {} textures loaded.\n", loadedPath, getTextureCount());
    setGraphicFileLoaded(true);
}

void AssetsManager::onDatLoaded(const std::string& loadedPath) {
    fmt::print("Finished loading dat from {}\n", loadedPath);
    fmt::print("Total: {} items, {} outfits, {} effects, {} missiles loaded.\n", 
               Items::getItemTypesCount(), 
               Outfits::getOutfitTypesCount(),
               Effects::getEffectTypesCount(),
               Missiles::getMissileTypesCount());

    // Only create preview textures for valid items
    // Skip item ID 0 if it's invalid, start from first valid item
    int firstValidItem = 0;
    int buttonsPerPage = ConfigManager::getInstance()->getButtonsCountItemPage();
    
    // Find first valid item
    for (uint32_t i = 0; i < Items::getItemTypesCount(); ++i) {
        auto it = Items::getItemType(i);
        if (it && it->width > 0 && it->height > 0) {
            firstValidItem = static_cast<int>(i);
            break;
        }
    }
    
    int lastItem = std::min(firstValidItem + buttonsPerPage, static_cast<int>(Items::getItemTypesCount()) - 1);
    if (firstValidItem <= lastItem) {
        createPreviewTexturesForPage(firstValidItem, lastItem, ThingCategory::ITEM);
    }

    // Create preview textures for outfits, effects, and missiles
    if (Outfits::getOutfitTypesCount() > 0) {
        int firstOutfit = 0;
        int lastOutfit = std::min(firstOutfit + buttonsPerPage, static_cast<int>(Outfits::getOutfitTypesCount()) - 1);
        if (firstOutfit <= lastOutfit) {
            createPreviewTexturesForPage(firstOutfit, lastOutfit, ThingCategory::OUTFIT);
        }
    }
    
    if (Effects::getEffectTypesCount() > 0) {
        int firstEffect = 0;
        int lastEffect = std::min(firstEffect + buttonsPerPage, static_cast<int>(Effects::getEffectTypesCount()) - 1);
        if (firstEffect <= lastEffect) {
            createPreviewTexturesForPage(firstEffect, lastEffect, ThingCategory::EFFECT);
        }
    }
    
    if (Missiles::getMissileTypesCount() > 0) {
        int firstMissile = 0;
        int lastMissile = std::min(firstMissile + buttonsPerPage, static_cast<int>(Missiles::getMissileTypesCount()) - 1);
        if (firstMissile <= lastMissile) {
            createPreviewTexturesForPage(firstMissile, lastMissile, ThingCategory::MISSILE);
        }
    }

    setDatFileLoaded(true);
}

void AssetsManager::buttonLoadGraphics(std::string& foundGraphicFilePath) {
    if(isGraphicFileLoaded()) {
        unload();
    }

    SavedData::getInstance()->setDataString("tempLoadedGraphicFilePath", foundGraphicFilePath);

    loadSpr(foundGraphicFilePath);
    Tools::removeSuffix(foundGraphicFilePath, ".spr");

    loadOTDat(foundGraphicFilePath + ".dat");
    SavedData::getInstance()->setDataString("tempLoadedDatFilePath", foundGraphicFilePath + ".dat");
}

void AssetsManager::loadThingTypePatterns(std::istream& inFile, std::shared_ptr<ThingType> thingType) {
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
    if (isAnimation && m_assetsInfo.frameDurations) {
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
        if (m_assetsInfo.extended) {
            uint8_t bytes[4];
            inFile.read(reinterpret_cast<char*>(bytes), 4);
            if (inFile.gcount() != 4) break;
            spriteId = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
        } else {
            uint8_t bytes[2];
            inFile.read(reinterpret_cast<char*>(bytes), 2);
            if (inFile.gcount() != 2) break;
            spriteId = bytes[0] | (bytes[1] << 8);
        }
        thingType->textureIdsVector[i] = spriteId;
    }
}

void AssetsManager::writeThingTypePatterns(std::ostream& outFile, std::shared_ptr<ThingType> thingType) {
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
    if (isAnimation && m_assetsInfo.frameDurations) {
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
        if (m_assetsInfo.extended) {
            outFile.write(reinterpret_cast<const char*>(&spriteId), sizeof(spriteId));
        } else {
            uint16_t spriteId16 = static_cast<uint16_t>(spriteId);
            outFile.write(reinterpret_cast<const char*>(&spriteId16), sizeof(spriteId16));
        }
    }

    // Fill remaining sprites with 0 if vector is smaller than expected
    for (uint32_t i = thingType->textureIdsVector.size(); i < numSprites; ++i) {
        if (m_assetsInfo.extended) {
            uint32_t zero = 0;
            outFile.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
        } else {
            uint16_t zero = 0;
            outFile.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
        }
    }
}

void AssetsManager::unloadDat() {
    Items::clearItemTypes();
    Outfits::clearOutfitTypes();
    Effects::clearEffectTypes();
    Missiles::clearMissileTypes();
    clearPreviewTextures();
}

void AssetsManager::unloadTextures() {
    textures.clear();
    textures.shrink_to_fit();
}

void AssetsManager::compile(const std::string& outputFilesPath) {
    std::string compileAssetsTo = outputFilesPath;
    std::string compileDatTo = outputFilesPath;
    if(compileAssetsTo.empty()) {
        compileAssetsTo = SavedData::getInstance()->getDataString("tempLoadedGraphicFilePath");
        compileDatTo = SavedData::getInstance()->getDataString("tempLoadedDatFilePath");
    }

    // Compile graphics and time them
    std::string pathWeCompiledGraphicsTo;
    auto start = std::chrono::high_resolution_clock::now();

    Tools::removeSuffix(compileAssetsTo, ".spr");
    pathWeCompiledGraphicsTo = compileAssetsTo + ".spr";
    compileSprFromTextures(pathWeCompiledGraphicsTo);

    auto end = std::chrono::high_resolution_clock::now(); // End time
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    fmt::print("Compiled graphics to: {}\nIt took: {}\n", pathWeCompiledGraphicsTo, Tools::formatDuration(duration));

    // Compile Dat
    Tools::removeSuffix(compileDatTo, ".dat");
    compileOTDat(compileDatTo + ".dat");
    fmt::print("Compiled dat to: {}\n", compileDatTo + ".dat");

    setUnsavedChanges(CATEGORY_MAIN_ONES, false);
}

void AssetsManager::unload() {
    setGraphicFileLoaded(false);
    setDatFileLoaded(false);
    unloadDat();
    unloadTextures();
}

void AssetsManager::doPopupAssetFileOpen() {
    auto sprFolderPath = SavedData::getInstance()->getDataString("sprFolderPath");
    bool foundOTDat = Tools::isPresentFileExtensionInAPath(sprFolderPath, ".dat");
    bool foundOTAssetsInFolder = foundOTDat && Tools::isPresentFileExtensionInAPath(sprFolderPath, ".spr");

    ImGui::Text("Spr Folder:");
    if (!Tools::isValidFolderPath(sprFolderPath)) {
        ImGui::SameLine();
        ImGui::Text("Invalid path!");
    } else if(!foundOTAssetsInFolder) {
        ImGui::SameLine();
        ImGui::Text("Missing .spr and/or .dat!");
    }

    ImGui::PushItemWidth(200);
    if(ImGui::InputText("##folderSpr", &sprFolderPath)) {
        SavedData::getInstance()->setDataString("sprFolderPath", sprFolderPath);
    };
    ImGui::SameLine();
    if (ImGui::Button("Browse##SelectPathToSprLoad")) {
        auto selectedFolder = Tools::openFileDialogChooseFolder();
        if (!selectedFolder.empty()) {
            SavedData::getInstance()->setDataString("sprFolderPath", selectedFolder);
        }
    }
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Options:");
    if (ImGui::Checkbox("Extended##ExtendedSprites", &m_assetsInfo.extended)) {
        SavedData::getInstance()->setDataBool("sprExtended", m_assetsInfo.extended);
    }
    if (ImGui::Checkbox("Transparency##TransparencySprites", &m_assetsInfo.transparency)) {
        SavedData::getInstance()->setDataBool("sprTransparency", m_assetsInfo.transparency);
    }
    ImGui::Checkbox("Frame Durations##FrameDurationsSpr", &m_assetsInfo.frameDurations);
    ImGui::Checkbox("Frame Groups##FrameGroupsSpr", &m_assetsInfo.frameGroups);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    float bottomOffset = ImGui::GetContentRegionAvail().y - ImGui::CalcTextSize("SomeTextForSize").y - ImGui::GetStyle().FramePadding.y * 2;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + bottomOffset);

    ImGui::SameLine();
    auto colorsCount2 = Tools::pushImGuiGray(!Tools::isValidFolderPath(sprFolderPath) || !foundOTAssetsInFolder);
    if (ImGui::Button("Load Spr")) {
        if (Tools::isValidFolderPath(sprFolderPath) && foundOTAssetsInFolder) {
            auto foundFile = Tools::findFile(sprFolderPath, ".spr");
            buttonLoadGraphics(foundFile);
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::PopStyleColor(colorsCount2);

    ImGui::SameLine();
    float offset = ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Cancel").x - ImGui::GetStyle().FramePadding.x * 2;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
    if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void AssetsManager::doPopupNewAssetFiles() {
    auto versions = getVersionsArray();
    std::string currentVLabel = "v" + std::string(versions[m_assetsInfo.versionIndex]);
    // trim trailing zeroes from float formatting
    currentVLabel.erase(currentVLabel.find_last_not_of('0') + 1, std::string::npos);
    if (currentVLabel.back() == '.') currentVLabel.pop_back();

    ImGui::Text("Select Version");
    if (ImGui::BeginCombo("##SelectNewAssetsVersion", currentVLabel.c_str())) {
        for (int i = 0; i < getVersionsArraySize(); ++i) {
            std::string itemLabel = "v" + std::string(versions[i]);

            itemLabel.erase(itemLabel.find_last_not_of('0') + 1, std::string::npos);
            if (itemLabel.back() == '.') itemLabel.pop_back();

            bool isSelected = (i == m_assetsInfo.versionIndex);
            if (ImGui::Selectable(itemLabel.c_str(), isSelected)) {
                m_assetsInfo.versionIndex = i;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::NewLine();
    ImGui::Text("Sprite Dimension");

    auto& spriteDimensions = getSpriteDimensionsVector();
    std::string currentDimLabel = std::to_string(spriteDimensions[m_assetsInfo.dimensionIndex]) + "x" + std::to_string(spriteDimensions[m_assetsInfo.dimensionIndex]);

    if (ImGui::BeginCombo("##SelectNewAssetsDimension", currentDimLabel.c_str())) {
        for (int i = 0; i < spriteDimensions.size(); ++i) {
            std::string itemLabel = std::to_string(spriteDimensions[i]) + "x" + std::to_string(spriteDimensions[i]);
            bool isSelected = (i == m_assetsInfo.dimensionIndex);
            if (ImGui::Selectable(itemLabel.c_str(), isSelected)) {
                m_assetsInfo.dimensionIndex = i;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::NewLine();
    ImGui::Text("Options:");
    ImGui::Checkbox("Extended", &m_tempCreation_AssetsInfo.extended);
    ImGui::Checkbox("Transparency", &m_tempCreation_AssetsInfo.transparency);
    ImGui::Checkbox("Frame Durations", &m_tempCreation_AssetsInfo.frameDurations);
    ImGui::Checkbox("Frame Groups", &m_tempCreation_AssetsInfo.frameGroups);

    ImGui::Separator();

    // Align buttons to the right
    float buttonWidth = 80.0f; // Width of each button
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float totalWidth = buttonWidth * 2 + spacing;
    ImGui::NewLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - totalWidth);

    if (ImGui::Button("Confirm", ImVec2(buttonWidth, 0))) {
        m_assetsInfo = m_tempCreation_AssetsInfo;

        unload();
        setGraphicFileLoaded(true);
        setDatFileLoaded(true);

        createNewTexture();
        // TO-DO use addItemType() from ItemsScrollableWindow instead
        auto newItemType = std::make_shared<ItemType>();
        Items::pushItemType(newItemType);
        // TO-DO select 1st itemType, also with  selectItem() from ItemsScrollableWindow

        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0))) {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

bool AssetsManager::isCompilable(bool needPath) {
    if(needPath && SavedData::getInstance()->getDataString("tempLoadedGraphicFilePath").empty()) {
        return false;
    }

    return hasUnsavedChanges(CATEGORY_MAIN_ONES);
}

void AssetsManager::doPopupAssetsCompileAs() {
    ImGui::Text("Name:");
    ImGui::InputText("##Name", m_assetsInfo.name, sizeof(m_assetsInfo.name));

    ImGui::Spacing();

    ImGui::Text("Output Folder:");
    ImGui::InputText("##OutputPath", &m_assetsInfo.outputPath);
    ImGui::SameLine();
    if (ImGui::Button("Browse")) {
        auto selectedFolder = Tools::openFileDialogChooseFolder();
        if (!selectedFolder.empty()) {
            m_assetsInfo.outputPath = selectedFolder;
        }
    }

    ImGui::Spacing();

    ImGui::Text("Version:");
    const char* currentItem = getVersionsArray()[m_assetsInfo.versionIndex];
    if (ImGui::BeginCombo("##VersionCombo", currentItem)) {
        for (int i = 0; i < getVersionsArraySize(); ++i) {
            bool selected = (m_assetsInfo.versionIndex == i);
            if (ImGui::Selectable(getVersionsArray()[i], selected)) {
                m_assetsInfo.versionIndex = i;
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::Spacing();
    ImGui::Text("Options");

    ImGui::Checkbox("Extended", &m_assetsInfo.extended);
    ImGui::Checkbox("Transparency", &m_assetsInfo.transparency);
    ImGui::Checkbox("Frame Durations", &m_assetsInfo.frameDurations);
    ImGui::Checkbox("Frame Groups", &m_assetsInfo.frameGroups);

    ImGui::Spacing();
    ImGui::Separator();

    if (ImGui::Button("Confirm", ImVec2(120, 0))) {
        compile(m_assetsInfo.outputPath + "\\" + m_assetsInfo.name);
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}
