#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

#include "AssetsManager.h"
#include "../Helper/SavedData.h"
#include "misc/cpp/imgui_stdlib.h"
#include "../Misc/definitions.h"
#include "../Misc/Timer.h"

AssetsManager::AssetsManager(GUIHelper* guiHelper) {
    this->guiHelper = guiHelper;

    // Setup blank texture
    auto spriteMaxSize = ConfigManager::getInstance()->getSpriteMaxSize();
    sf::Image image({spriteMaxSize, spriteMaxSize}, sf::Color::Transparent);
    BLANK_TEXTURE = std::make_shared<sf::Texture>();
    if (!BLANK_TEXTURE->loadFromImage(image)) {
        Warninger::sendWarning(FUNC_NAME, "Failed to create blank texture.");
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
    for (uint32_t spriteId = 1; spriteId <= spriteCount; ++spriteId) {
        uint32_t offset = offsets[spriteId - 1];
        if (offset == 0) continue;

        file.seekg(offset, std::ios::beg);
        file.ignore(3); // Skip unused bytes

        // Read sprite data size
        uint16_t dataSize;
        file.read(reinterpret_cast<char*>(&dataSize), 2);

        // Read compressed sprite data
        std::vector<uint8_t> spriteData(dataSize);
        file.read(reinterpret_cast<char*>(spriteData.data()), dataSize);

        // Process RLE data into 32x32 RGBA pixels
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
                uint8_t a = m_assetsInfo.transparency ? spriteData[dataPtr++] : 255;

                // Fill RGBA buffer
                size_t idx = pixelPtr * 4;
                pixels[idx] = r;
                pixels[idx + 1] = g;
                pixels[idx + 2] = b;
                pixels[idx + 3] = a;

                pixelPtr++;
            }
        }

        auto texture = std::make_shared<sf::Texture>(sf::Vector2u(singleSpriteSize, singleSpriteSize));
        if (texture->getSize().x != 0 && texture->getSize().y != 0) {
            texture->update(pixels.data());
            textures.push_back(texture);
        } else {
            //Warninger::sendWarning(FUNC_NAME, "Failed to create texture, id: " + std::to_string(textureId));
        }
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
        return (ImTextureID)BLANK_TEXTURE->getNativeHandle();
    }

    auto texture = textures.at(id);
    return (ImTextureID)texture->getNativeHandle();
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
    fmt::println("--- Compiling .dat needs to be implemented :) TO-DO");
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

        // Skip outfit count, effect count, and missile count (2 bytes each)
        inFile.seekg(6, std::ios_base::cur);

        // Read items starting from ID 100
        for (uint16_t id = 0; id <= itemCount - 100; ++id) {
            auto itemType = std::make_shared<ItemType>();

            // Read flags until we encounter 0xFF (ItemFlag.LastFlag)
            uint8_t flag;
            while (true) {
                inFile.read(reinterpret_cast<char*>(&flag), sizeof(flag));
                //std::cout << "Read flag 0x" << std::hex << (int)flag << " at id " <<  std::dec << id << std::endl;
                if (flag == 0xFF) break; // LastFlag

                switch (flag) {
                    case 0x00: // Ground
                        //itemType->type = ITEM_TYPE_GROUND;
                        inFile.read(reinterpret_cast<char*>(&itemType->speed), sizeof(itemType->speed));
                        break;
                    case 0x01: // GroundBorder
                        //itemType->hasStackOrder = true;
                        //itemType->stackOrder = STACK_ORDER_BORDER;
                        break;
                    case 0x02: // OnBottom
                        //itemType->hasStackOrder = true;
                        //itemType->stackOrder = STACK_ORDER_BOTTOM;
                        break;
                    case 0x03: // OnTop
                        //itemType->hasStackOrder = true;
                        //itemType->stackOrder = STACK_ORDER_TOP;
                        break;
                    case 0x04: // Container
                        //itemType->type = ITEM_TYPE_CONTAINER;
                        break;
                    case 0x05: // Stackable
                        //itemType->stackable = true;
                        break;
                    case 0x06: // ForceUse
                        //itemType->forceUse = true;
                        break;
                    case 0x07: // MultiUse
                        //itemType->multiUse = true;
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
                        //itemType->unpassable = true;
                        break;
                    case 0x0D: // Unmoveable
                        //itemType->unmoveable = true;
                        break;
                    case 0x0E: // BlockMissiles
                        //itemType->blockMissiles = true;
                        break;
                    case 0x0F: // BlockPathfinder
                        //itemType->blockPathfinder = true;
                        break;
//                    case 0x10: // NoMoveAnimation
//                        // Not implemented
//                        break;
                    case 0x10: // Pickupable
                        //itemType->pickupable = true;
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

            // Read data
            inFile.read(reinterpret_cast<char*>(&itemType->width), sizeof(itemType->width));
            inFile.read(reinterpret_cast<char*>(&itemType->height), sizeof(itemType->height));

            if (itemType->width > 1 || itemType->height > 1) {
                inFile.seekg(1, std::ios_base::cur); // Skip exact size
            }

            inFile.read(reinterpret_cast<char*>(&itemType->layers), sizeof(itemType->layers));
            inFile.read(reinterpret_cast<char*>(&itemType->patternX), sizeof(itemType->patternX));
            inFile.read(reinterpret_cast<char*>(&itemType->patternY), sizeof(itemType->patternY));
            inFile.read(reinterpret_cast<char*>(&itemType->patternZ), sizeof(itemType->patternZ));
            inFile.read(reinterpret_cast<char*>(&itemType->animationsFrames), sizeof(itemType->animationsFrames));
            bool isAnimation = itemType->animationsFrames > 1;

            // Skip frame durations if needed
            if (isAnimation && m_assetsInfo.frameDurations) {
                inFile.seekg(6 + 8 * itemType->animationsFrames, std::ios_base::cur);
            }

            // Calculate number of sprites
            uint32_t numSprites = itemType->width * itemType->height * itemType->layers *
                    itemType->patternX * itemType->patternY * itemType->patternZ *
                                  itemType->animationsFrames;

            // Read sprite IDs
            itemType->textureIdsVector.resize(numSprites);
            for (uint32_t i = 0; i < numSprites; ++i) {
                uint32_t spriteId = 0;
                if (m_assetsInfo.extended) {
                    uint32_t id32;
                    inFile.read(reinterpret_cast<char*>(&id32), sizeof(id32));
                    spriteId = id32;
                } else {
                    uint16_t id16;
                    inFile.read(reinterpret_cast<char*>(&id16), sizeof(id16));
                    spriteId = id16;
                }

                itemType->textureIdsVector[i] = spriteId;
            }

            // Store the item
            Items::pushItemType(itemType);
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

std::shared_ptr<sf::Texture> AssetsManager::getPreviewTexture(int itemTypeId) {
    if(itemTypeId < previewTextures.size() && previewTextures.at(itemTypeId)) {
        return previewTextures[itemTypeId];
    }

    return BLANK_TEXTURE;
}

void AssetsManager::replacePreviewTexture(int itemTypeId, std::shared_ptr<sf::Texture> texture) {
    if(itemTypeId < 0 || itemTypeId >= previewTextures.size()) {
        return;
    }

    previewTextures[itemTypeId] = texture;
}

void AssetsManager::createPreviewTexture(int id) {
    auto itemPreviewTexture = getItemSpriteSheet(id, 1);
    auto finalSprite = sf::Sprite(itemPreviewTexture);

    // Scale to the Config defined sprite's desired button size
    auto SizeOfButtonSprite = ConfigManager::getInstance()->getSpriteButtonSize();
    finalSprite.setScale({
         static_cast<float>(SizeOfButtonSprite.x) / itemPreviewTexture.getSize().x,
         static_cast<float>(SizeOfButtonSprite.y) / itemPreviewTexture.getSize().y
     });

    // Create a new RenderTexture to hold the scaled image
    sf::RenderTexture scaledRender (sf::Vector2u(SizeOfButtonSprite.x, SizeOfButtonSprite.y));
    scaledRender.clear(sf::Color::Transparent);
    scaledRender.draw(finalSprite);
    scaledRender.display();

    auto texture = std::make_shared<sf::Texture>(scaledRender.getTexture());

    if (id >= previewTextures.size()) {
        previewTextures.push_back(texture);
    } else {
        replacePreviewTexture(id, texture);
    }
}

void AssetsManager::createPreviewTexturesForPage(int pageFirstItemType, int pageLastItemType) {
    for (int id = pageFirstItemType; id <= pageLastItemType; ++id) {
        createPreviewTexture(id);
    }
}

void AssetsManager::clearPreviewTextures() {
    previewTextures.clear();
    previewTextures.shrink_to_fit();
}

sf::Texture AssetsManager::getItemSpriteSheet(int itemTypeId, int animations) {
    if (!Items::isValidItemTypeIndex(itemTypeId)) {
        Warninger::sendWarning(FUNC_NAME, "Invalid itemType id: " + std::to_string(itemTypeId));
        return sf::Texture(*BLANK_TEXTURE);
    }

    auto it = Items::getItemType(itemTypeId);
    if (!it) {
        Warninger::sendWarning(FUNC_NAME, "Couldn't get ItemType (" + std::to_string(itemTypeId) + ")");
        return sf::Texture(*BLANK_TEXTURE);
    }

    auto spriteMaxSize = ConfigManager::getInstance()->getSpriteMaxSize();
    int singleAnimationFrameSize = it->height * spriteMaxSize;

    // Create a RenderTexture with the correct size
    sf::Vector2u size{
            static_cast<unsigned>(it->width * spriteMaxSize),
            static_cast<unsigned>(singleAnimationFrameSize * animations)
    };
    sf::RenderTexture render(size);
    if (render.getSize().x == 0 || render.getSize().y == 0) {
        Warninger::sendWarning(FUNC_NAME, "Failed to create RenderTexture for ItemType (" + std::to_string(itemTypeId) + ")");
        return sf::Texture(*BLANK_TEXTURE);
    }

    render.clear(sf::Color::Transparent);

    for (int a = 1; a <= animations; a++) {
        for (int y = 0; y < it->height; y++) {
            for (int x = 0; x < it->width; x++) {
                auto texture = getTexture(getTextureIdFromItemType(it, y, x, a));

                if (texture) {
                    sf::Sprite sprite(*texture);
                    sprite.setPosition(sf::Vector2f(
                            static_cast<float>(x * spriteMaxSize),
                            static_cast<float>(y * spriteMaxSize + ((a - 1) * singleAnimationFrameSize))
                    ));
                    render.draw(sprite);
                }
            }
        }
    }
    render.display();

    // Careful - store texture in a heap-allocated object to persist after function exit
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
    auto colorsCount = pushImGuiGray(!isCompilable());
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
                : cleanPathIntoFolderPath(SavedData::getInstance()->getDataString("tempLoadedGraphicFilePath"));
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
    fmt::print("Finished loading dat from {}\nTotal: {} itemTypes loaded.\n", loadedPath, Items::getItemTypesCount());

    createPreviewTexturesForPage(0, ConfigManager::getInstance()->getButtonsCountItemPage());

    setDatFileLoaded(true);
}

void AssetsManager::buttonLoadGraphics(std::string& foundGraphicFilePath) {
    if(isGraphicFileLoaded()) {
        unload();
    }

    SavedData::getInstance()->setDataString("tempLoadedGraphicFilePath", foundGraphicFilePath);

    loadSpr(foundGraphicFilePath);
    removeSuffix(foundGraphicFilePath, ".spr");

    loadOTDat(foundGraphicFilePath + ".dat");
    SavedData::getInstance()->setDataString("tempLoadedDatFilePath", foundGraphicFilePath + ".dat");
}

void AssetsManager::unloadDat() {
    Items::clearItemTypes();
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

    removeSuffix(compileAssetsTo, ".spr");
    pathWeCompiledGraphicsTo = compileAssetsTo + ".spr";
    compileSprFromTextures(pathWeCompiledGraphicsTo);

    auto end = std::chrono::high_resolution_clock::now(); // End time
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    fmt::print("Compiled graphics to: {}\nIt took: {}\n", pathWeCompiledGraphicsTo, formatDuration(duration));

    // Compile Dat
    removeSuffix(compileDatTo, ".dat");
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
    bool foundOTDat = isPresentFileExtensionInAPath(sprFolderPath, ".dat");
    bool foundOTAssetsInFolder = foundOTDat && isPresentFileExtensionInAPath(sprFolderPath, ".spr");

    ImGui::Text("Spr Folder:");
    if (!isValidFolderPath(sprFolderPath)) {
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
        auto selectedFolder = openFileDialogChooseFolder();
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
    auto colorsCount2 = pushImGuiGray(!isValidFolderPath(sprFolderPath) || !foundOTAssetsInFolder);
    if (ImGui::Button("Load Spr")) {
        if (isValidFolderPath(sprFolderPath) && foundOTAssetsInFolder) {
            auto foundFile = findFile(sprFolderPath, ".spr");
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
    ImGui::InputText("##OutputPath", &m_assetsInfo.outputPath, m_assetsInfo.outputPath.size());
    ImGui::SameLine();
    if (ImGui::Button("Browse")) {
        auto selectedFolder = openFileDialogChooseFolder();
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
