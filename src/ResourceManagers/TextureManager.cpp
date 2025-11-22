#include "TextureManager.h"
#include "ConfigManager.h"
#include "../Misc/Warninger.h"
#include "../Misc/definitions.h"
#include <filesystem>
#include <fmt/format.h>

TextureManager::TextureManager() {
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
}

std::shared_ptr<sf::Texture> TextureManager::getTexture(int id) {
    if(!isValidTextureIndex(id)) {
        return BLANK_TEXTURE;
    }
    return textures.at(id);
}

ImTextureID TextureManager::getImGuiTexture(int id) {
    if(!isValidTextureIndex(id)) {
        return (ImTextureID)BLANK_TEXTURE->getNativeHandle();
    }
    auto texture = textures.at(id);
    return (ImTextureID)texture->getNativeHandle();
}

bool TextureManager::isValidTexture(std::shared_ptr<sf::Texture> texture) {
    if (!texture) {
        return false;
    }

    auto spriteMaxSize = ConfigManager::getInstance()->getSpriteMaxSize();
    auto textureSize = texture->getSize();

    // Check if texture dimensions match expected size
    if (textureSize.x != spriteMaxSize || textureSize.y != spriteMaxSize) {
        return false;
    }

    return true;
}

bool TextureManager::isValidTextureIndex(int id) {
    if (id < 0 || id >= textures.size()) {
        return false;
    }
    // Check if the pointer is not null
    return textures.at(id) != nullptr;
}

bool TextureManager::pushTexture(std::shared_ptr<sf::Texture> texture) {
    if(!isValidTexture(texture)) {
        return false;
    }
    textures.push_back(texture);
    return true;
}

void TextureManager::replaceTexture(int id, std::shared_ptr<sf::Texture> newTexture) {
    if(!isValidTextureIndex(id)) {
        Warninger::sendErrorMsg(FUNC_NAME, "Invalid texture id " + std::to_string(id));
        return;
    }
    textures[id] = newTexture;
}

void TextureManager::removeTexture(int id) {
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

void TextureManager::createNewTexture() {
    textures.push_back(BLANK_TEXTURE);
}

void TextureManager::clear() {
    textures.clear();
}

void TextureManager::exportTexture(const std::string& outputString, int textureId) {
    auto texture = getTexture(textureId);
    exportTexture(outputString, *texture);
}

void TextureManager::exportTexture(const std::string& outputString, const sf::Texture& texture) {
    sf::Image image = texture.copyToImage();

    if (image.saveToFile(outputString)) {
        fmt::print("Image saved successfully: {}\n", outputString);
        return;
    } else {
        Warninger::sendWarning(FUNC_NAME, "Failed to save image: " + outputString);
        return;
    }
}

