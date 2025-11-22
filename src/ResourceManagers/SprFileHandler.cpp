#include "SprFileHandler.h"
#include "TextureManager.h"
#include "AssetsManager.h" // For AssetsInfo definition
#include "ConfigManager.h"
#include "../Misc/Warninger.h"
#include "../Misc/definitions.h"
#include "../Misc/Timer.h"
#include <fstream>
#include <vector>
#include <SFML/Graphics.hpp>
#include <fmt/format.h>

SprFileHandler::SprFileHandler(TextureManager* textureManager, AssetsInfo* assetsInfo, AssetsManager* assetsManager)
    : textureManager(textureManager), assetsInfo(assetsInfo), assetsManager(assetsManager) {
}

uint16_t SprFileHandler::readLE16(const uint8_t* data) {
    return data[0] | (data[1] << 8);
}

void SprFileHandler::writeLE16(std::vector<uint8_t>& data, uint16_t val) {
    data.push_back(val & 0xFF);
    data.push_back((val >> 8) & 0xFF);
}

bool SprFileHandler::loadSpr(const std::string& sprFilePath, uint32_t& loadedSignature) {
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
    loadedSignature = signature;

    // Read sprite count (2 bytes for non-extended format)
    uint32_t spriteCount;
    if(assetsInfo->extended) {
        file.read(reinterpret_cast<char *>(&spriteCount), 4);
    } else {
        uint16_t tempSpriteCount;
        file.read(reinterpret_cast<char*>(&tempSpriteCount), 2);
        spriteCount = static_cast<uint32_t>(tempSpriteCount);
    }

    // Add BLANK_TEXTURE, as air (id 0)
    textureManager->getTextures().reserve(1 + spriteCount);
    textureManager->getTextures().push_back(textureManager->getBlankTexture());

    // Read sprite offsets (4 bytes per offset)
    std::vector<uint32_t> offsets(spriteCount);
    for (uint32_t i = 0; i < spriteCount; ++i) {
        file.read(reinterpret_cast<char*>(&offsets[i]), 4);
    }

    // temp var to decide loaded sprite size
    // Get sprite dimensions from AssetsManager if available, otherwise use spriteMaxSize
    int singleSpriteSize = ConfigManager::getInstance()->getSpriteMaxSize();
    if (assetsManager) {
        const auto& spriteDimensions = assetsManager->getSpriteDimensionsVector();
        if (assetsInfo->dimensionIndex >= 0 && assetsInfo->dimensionIndex < static_cast<int>(spriteDimensions.size())) {
            singleSpriteSize = spriteDimensions.at(assetsInfo->dimensionIndex);
        }
    }

    // Process each sprite
    // IMPORTANT: We must push a texture for EVERY sprite ID, even if offset is 0 or sprite is empty
    // This maintains the correct mapping: sprite ID in .dat file = texture index in our vector
    for (uint32_t spriteId = 1; spriteId <= spriteCount; ++spriteId) {
        uint32_t offset = offsets[spriteId - 1];
        std::shared_ptr<sf::Texture> texture;
        
        if (offset == 0) {
            // Sprite doesn't exist (offset 0), push blank texture to maintain index mapping
            texture = textureManager->getBlankTexture();
        } else {
            file.seekg(offset, std::ios::beg);
            if (!file.good()) {
                Warninger::sendWarning(FUNC_NAME, "Failed to seek to offset for sprite " + std::to_string(spriteId) + ". Using blank texture.");
                texture = textureManager->getBlankTexture();
            } else {
                file.ignore(3); // Skip unused bytes (RGB)

                // Read sprite data size
                uint16_t dataSize;
                file.read(reinterpret_cast<char*>(&dataSize), 2);
                
                if (!file.good() || dataSize == 0) {
                    // Empty sprite or read error, use blank texture
                    texture = textureManager->getBlankTexture();
                } else {
                    // Read compressed sprite data
                    std::vector<uint8_t> spriteData(dataSize);
                    file.read(reinterpret_cast<char*>(spriteData.data()), dataSize);
                    
                    if (!file.good()) {
                        Warninger::sendWarning(FUNC_NAME, "Failed to read sprite data for sprite " + std::to_string(spriteId) + ". Using blank texture.");
                        texture = textureManager->getBlankTexture();
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
                                uint8_t a = assetsInfo->transparency ? (dataPtr < spriteData.size() ? spriteData[dataPtr++] : 255) : 255;

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
                        sf::Image image(sf::Vector2u(singleSpriteSize, singleSpriteSize), pixels.data());
                        texture = std::make_shared<sf::Texture>();
                        if (texture->loadFromImage(image)) {
                            // Texture loaded successfully
                        } else {
                            Warninger::sendWarning(FUNC_NAME, "Failed to create texture for sprite " + std::to_string(spriteId) + ". Using blank texture.");
                            texture = textureManager->getBlankTexture();
                        }
                    }
                }
            }
        }
        
        // ALWAYS push a texture (even if blank) to maintain correct sprite ID to texture index mapping
        textureManager->getTextures().push_back(texture);
    }

    return true;
}

void SprFileHandler::compileSpr(const std::string& fileName, uint32_t signature) {
    // temp var for an optional feature that I once used
    bool downscale64To32 = false;

    std::ofstream out(fileName, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Failed to open file for writing: " << fileName << std::endl;
        return;
    }

    // 1. Write signature
    out.write(reinterpret_cast<const char*>(&signature), sizeof(signature));

    // 2. Write sprite count
    uint32_t spriteCount = static_cast<uint32_t>(textureManager->getTextureCount());
    if (assetsInfo->extended) {
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
        auto texture = textureManager->getTexture(i);
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
                if (assetsInfo->transparency) {
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

