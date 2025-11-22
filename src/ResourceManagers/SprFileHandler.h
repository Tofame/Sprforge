#pragma once

#include <string>
#include <cstdint>
#include <vector>

// Forward declarations
class TextureManager;
class AssetsManager; // For getSpriteDimensionsVector access
struct AssetsInfo; // Defined in AssetsManager.h

/**
 * @brief Handles .spr file loading and compilation
 * 
 * This class is responsible for:
 * - Loading .spr files into TextureManager
 * - Compiling textures from TextureManager into .spr files
 */
class SprFileHandler {
public:
    SprFileHandler(TextureManager* textureManager, AssetsInfo* assetsInfo, AssetsManager* assetsManager = nullptr);
    ~SprFileHandler() = default;

    /**
     * @brief Loads textures from a .spr file
     * 
     * @param sprFilePath Path to the .spr file (empty = default path)
     * @param loadedSignature Output parameter for the loaded signature
     * @return True if loading was successful
     */
    bool loadSpr(const std::string& sprFilePath, uint32_t& loadedSignature);

    /**
     * @brief Compiles textures from TextureManager into a .spr file
     * 
     * @param outputFilePath Path to output .spr file
     * @param signature Signature to write to the file
     */
    void compileSpr(const std::string& outputFilePath, uint32_t signature);

private:
    TextureManager* textureManager;
    AssetsInfo* assetsInfo;
    AssetsManager* assetsManager; // For getSpriteDimensionsVector access

    // Helper function to read a little-endian 16-bit integer
    static uint16_t readLE16(const uint8_t* data);
    
    // Helper function to write a little-endian 16-bit integer
    static void writeLE16(std::vector<uint8_t>& data, uint16_t val);
};

