#pragma once

#include <string>
#include <toml++/toml.h>
#include <iostream>
#include "../Misc/tools.h"

class ConfigManager {
public:
    static ConfigManager* getInstance() {
        if(instance_ == nullptr) {
            instance_ = new ConfigManager();
            instance_->loadConfig("config.toml");
        }

        return instance_;
    }

    void loadConfig(const std::string& filename);

    // Getter functions for the configuration values
    [[nodiscard]] uint16_t getSpriteMaxSize() const { return SPRITE_MAXSIZE; };
    [[nodiscard]] const ImVec2& getSpriteButtonSize() const { return BUTTONSIZE_SPRITE; }
    [[nodiscard]] const ImVec2& getItemButtonSize() const { return BUTTONSIZE_ITEM; }
    [[nodiscard]] int getButtonsCountItemPage() const { return BUTTONS_ITEMPAGE; };
    [[nodiscard]] int getButtonsCountSpritePage() const { return BUTTONS_SPRITEPAGE; };
    [[nodiscard]] ImU32 getImGuiGridColor() const { return Tools::ParseHexColor(GRID_COLOR); };
    [[nodiscard]] ImU32 getImGuiSelectedThingColor() const { return Tools::ParseHexColor(SELECTED_THING_COLOR); };

    [[nodiscard]] int getItemMaxWidth() const { return ITEM_MAXWIDTH; };
    [[nodiscard]] int getItemMaxHeight() const { return ITEM_MAXHEIGHT; };
    [[nodiscard]] int getItemMaxAnimationCount() const { return ITEM_MAXANIMATIONS; };

    [[nodiscard]] const std::string& getAssetsFileName() const { return FILE_ASSETS_NAME; }
    [[nodiscard]] const std::string& getDatFileName() const { return FILE_ITEMS_NAME; }

    [[nodiscard]] const std::string& getPathAssets() const { return PATH_ASSETS; }
private:
    static ConfigManager* instance_;

    ConfigManager() = default;

    // Prevent copying and assignment
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // Configuration variables
    uint16_t SPRITE_MAXSIZE;
    ImVec2 BUTTONSIZE_SPRITE;
    ImVec2 BUTTONSIZE_ITEM;
    int BUTTONS_ITEMPAGE;
    int BUTTONS_SPRITEPAGE;
    std::string GRID_COLOR;
    std::string SELECTED_THING_COLOR;

    int ITEM_MAXWIDTH;
    int ITEM_MAXHEIGHT;
    int ITEM_MAXANIMATIONS;

    std::string FILE_ASSETS_NAME;
    std::string FILE_ITEMS_NAME;

    std::string PATH_ASSETS;
};