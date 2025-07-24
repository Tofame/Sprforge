#pragma once

#include <unordered_map>
#include <filesystem>
#include <iostream>
#include "SFML/Graphics/Texture.hpp"
#include "imgui.h"

class GUIHelper {
public:
    GUIHelper() {
        std::string basicPath = std::filesystem::current_path().string() + "/data/images/ui/";

        loadTextures(basicPath + "icons/");
    }

    ~GUIHelper() {
        for (auto& pair : guiTextures) {
            delete pair.second;
        }
    }

    ImTextureID getImGuiTexture(const std::string& name) {
        auto it = guiTextures.find(name);
        if (it != guiTextures.end()) {
            auto texture = it->second;
            return (ImTextureID)texture->getNativeHandle(); // Return the pointer to the texture if found
        }

        throw std::runtime_error("[GUIHelper::getTexture()] Cannot load a texture called: " + name);
    }

private:
    void loadTextures(const std::string& folderPath) {
        for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
            if (entry.path().extension() == ".png" || entry.path().extension() == ".jpg") {
                std::string filename = entry.path().stem().string();

                sf::Texture* texture = new sf::Texture();
                if (texture->loadFromFile(entry.path().string())) {
                    guiTextures[filename] = texture;
                } else {
                    delete texture;
                }
            }
        }
    }

    std::unordered_map<std::string, sf::Texture*> guiTextures;
};