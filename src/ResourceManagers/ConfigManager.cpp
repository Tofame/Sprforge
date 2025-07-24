#include "ConfigManager.h"

ConfigManager* ConfigManager::instance_ = nullptr;

void ConfigManager::loadConfig(const std::string &filename) {
    try {
        auto config = toml::parse_file(filename);

        // Load config sections
        auto guiConfig = config["GUI"];
        SPRITE_MAXSIZE = guiConfig["spriteSize"].value_or(32);

        int itemButtonSize = guiConfig["itemButtonSize"].value_or(32);
        BUTTONSIZE_ITEM = {static_cast<float>(itemButtonSize), static_cast<float>(itemButtonSize)};
        int spriteButtonSize = guiConfig["spriteButtonSize"].value_or(32);
        BUTTONSIZE_SPRITE = {static_cast<float>(spriteButtonSize), static_cast<float>(spriteButtonSize)};

        BUTTONS_ITEMPAGE = std::max(1, guiConfig["buttonsPerItemPage"].value_or(1));
        BUTTONS_SPRITEPAGE = std::max(1, guiConfig["buttonsPerSpritePage"].value_or(1));
        GRID_COLOR = guiConfig["gridColor"].value_or("#892ce6");
        SELECTED_THING_COLOR = guiConfig["selectedThingColor"].value_or("#FF6347");

        auto itemPropertiesConfig = config["ITEMPROPERTIES"];
        ITEM_MAXWIDTH = std::max(1, itemPropertiesConfig["maxWidth"].value_or(1));
        ITEM_MAXHEIGHT = std::max(1, itemPropertiesConfig["maxHeight"].value_or(1));
        ITEM_MAXANIMATIONS = std::max(1, itemPropertiesConfig["maxAnimationCount"].value_or(1));

        auto compileConfig = config["COMPILE"];
        FILE_ASSETS_NAME = compileConfig["assetsFileName"].value_or("default.spr");
        FILE_ITEMS_NAME = compileConfig["itemsFileName"].value_or("default.dat");

        auto pathConfig = config["PATHS"];
        PATH_ASSETS = pathConfig["assetsPath"].value_or("data/things/");
    } catch (const toml::parse_error& err) {
        std::cerr << "TOML Parse Error: " << err << std::endl;
    }
}