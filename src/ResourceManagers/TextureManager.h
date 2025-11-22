#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include "imgui.h"

/**
 * @brief Manages texture storage and retrieval
 * 
 * This class is responsible for:
 * - Storing textures in a vector
 * - Providing access to textures by ID
 * - Validating texture indices
 * - Managing a blank texture fallback
 */
class TextureManager {
public:
    TextureManager();
    ~TextureManager() = default;

    // Texture access
    std::vector<std::shared_ptr<sf::Texture>>& getTextures() { return textures; }
    [[nodiscard]] size_t getTextureCount() const { return textures.size(); }
    std::shared_ptr<sf::Texture> getTexture(int id);
    ImTextureID getImGuiTexture(int id);

    // Texture validation
    /**
     * @brief Checks if the given texture is valid based on predefined conditions.
     *
     * A texture is considered valid if:
     * - It has the correct dimensions as specified in the configuration file.
     *
     * @param texture Pointer to the sf::Texture to be validated.
     * @return True if the texture meets the validity criteria, false otherwise.
     */
    bool isValidTexture(std::shared_ptr<sf::Texture> texture);
    
    /**
     * @brief Checks if the given index is in-range of texture collection
     *
     * @param id Index Number
     * @return True if the index meets the validity criteria, false otherwise.
     */
    bool isValidTextureIndex(int id);

    // Texture manipulation
    bool pushTexture(std::shared_ptr<sf::Texture> texture);
    void replaceTexture(int id, std::shared_ptr<sf::Texture> newTexture);
    void removeTexture(int id);
    void createNewTexture();
    void clear();

    // Export
    void exportTexture(const std::string& outputString, int textureId);
    void exportTexture(const std::string& outputString, const sf::Texture& texture);

    // Blank texture access
    std::shared_ptr<sf::Texture> getBlankTexture() const { return BLANK_TEXTURE; }

private:
    std::vector<std::shared_ptr<sf::Texture>> textures;
    std::shared_ptr<sf::Texture> BLANK_TEXTURE;
};

