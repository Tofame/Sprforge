#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <SFML/Graphics.hpp>
#include <cstdint>
#include "../Things/ItemType.h"
#include "../Things/Items.h"
#include "imgui.h"
#include "ConfigManager.h"
#include "../Misc/Warninger.h"
#include "../Helper/GUIHelper.h"
#include "../Helper/SavedData.h"

enum ASSET_CATEGORY {
    CATEGORY_ITEMS = 0,
    CATEGORY_ITEMS_ITEMTYPE = 1,
    CATEGORY_SPRITES = 2,
    CATEGORY_OUTFITS = 3,
    CATEGORY_MAIN_ONES // especially important on changing unsaved changes on - sprites, items, outfits
};

struct AssetsInfo {
    uint8_t versionIndex = 0;
    uint8_t dimensionIndex = 0;

    bool extended = false;
    bool transparency = false;
    bool frameDurations = false; // also called 'improvedAnimations' I think?
    bool frameGroups = false;

    // Compiling stuff
    char name[128] = "Tibia"; // assets name without extension
    int compileTypeIndex = 1;
    std::string outputPath;
};

class AssetsManager {
public:
    explicit AssetsManager(GUIHelper* guiHelper);
    ~AssetsManager();

    std::vector<std::shared_ptr<sf::Texture>>& getTextures() { return textures; };
    [[nodiscard]] size_t getTextureCount() const { return textures.size(); };
    std::shared_ptr<sf::Texture> getTexture(int id);
    ImTextureID getImGuiTexture(int id);

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
    bool pushTexture(std::shared_ptr<sf::Texture> texture);
    void replaceTexture(int id, std::shared_ptr<sf::Texture> newTexture);
    void removeTexture(int id);
    void createNewTexture();

    // Compiles .spr file from loaded Textures into the app
    void compileSprFromTextures(const std::string& outputFilePath = "");
    // Main method for 'Compile' button, responsible for compiling .spr and .dat
    void compile(const std::string& outputFilesPath = "");
    void compileOTDat(const std::string& outputFilePath = "");

    // Loads textures from binary file containing graphics
    // Returns true if getTextureCount() is > 0.
    bool loadSpr(const std::string& sprFilePath = "");
    void loadOTDat(const std::string& datFilePath = "");

    // Unloads all - textures, dat etc.
    void unload();
    void unloadTextures();
    void unloadDat();

    void onGraphicsLoaded(const std::string& loadedPath);
    void onDatLoaded(const std::string& loadedPath);

    /**
     * @brief Helper method for detecting changes to Items, Sprites etc.
     *
     * Whenever changes are made to e.g. ItemType, if all is correct
     * the setUnsavedChanges() gets called. Then, with this method
     * we can check for the value of it, to for example detect 'Compile' possibility
     * or 'Save' itemtype etc.
     * CATEGORY_MAIN_ONES returns if either is changed: Outfit, Effect, ItemType
     *
     * @param fromCategory Category to check whether it is unsaved.
     * @return True if there are unsaved changes in that category
     */
    [[nodiscard]] bool hasUnsavedChanges(ASSET_CATEGORY fromCategory) const;
    /**
     * @brief Helper method for setting detected changes to Items, Sprites etc.
     *
     * Whenever changes are made to e.g. ItemType, if all is correct
     * this method gets called to save information about such situation.
     * We can later check with hasUnsavedChanges(), if for example assets can 'Compile'
     * or itemType can be 'Save' etc.
     * CATEGORY_MAIN_ONES param sets all main: Item, ItemType, Outfits, Effects to true.
     *
     * @param fromCategory Category to set to a bool value
     * @param value true/false to set to category
     */
    void setUnsavedChanges(ASSET_CATEGORY fromCategory, bool value);

    // Helper methods, the UnsavedItemType, is a thing
    // That we use to store current changes to ItemType.
    // Whenever prompted to save or not save changes, we will
    // either on 'Save', use this unsavedItemType to set it as an actual ItemType
    // or when canceling, we will just discard it.
    void setUnsavedItemType(const std::shared_ptr<ItemType>& itemTypeToCopy, int id) {
        if (itemTypeToCopy) {
            unsavedItemTypeCopy = std::make_shared<ItemType>(*itemTypeToCopy); // deep copy
            unsavedItemTypeId = id;
        } else {
            resetUnsavedItemType();
        }
    }
    void resetUnsavedItemType() {
        unsavedItemTypeCopy.reset();
        unsavedItemTypeId = -1;
    }
    std::shared_ptr<ItemType> getUnsavedItemType() {
        if (unsavedItemTypeCopy) {
            return unsavedItemTypeCopy;
        } else {
            // Create a brand new copy of dollItemType
            // This assumes ItemType has a copy constructor or a way to clone it.
            // For shared_ptr, you'll need to create a new object and then a new shared_ptr to it.
            return std::make_shared<ItemType>(*Items::dollItemType);
        }
    }
    [[nodiscard]] int getUnsavedItemTypeId() const {
        return unsavedItemTypeId;
    }

    [[nodiscard]] int getAnimationFrameSetting() const {
        return animationFrameSetting;
    }
    // ImGui slider uses it to track/update the value. So we needed a reference, to operate on the original
    int& getAnimationFrameSettingRef() {
        return animationFrameSetting;
    }
    void setAnimationFrameSetting(int id) {
        if(id < 0 || id > ConfigManager::getInstance()->getItemMaxAnimationCount()) {
            return;
        }
        animationFrameSetting = id;
    };

    /**
     * @brief Gets you texture id from the ItemType
     *
     * Every ItemType can be composed of more than 1 texture.
     * And so, when we want to know the id of a texture that item has
     * we can do that by accessing ItemType's vector that stores information about used texture ids.
     *
     * @param it itemType from which we will get basic information, such as it's max width/height
     * @param h cell's 'height' from which we want texture
     * @param w cell's 'width' from which we want texture
     * @param a cell's 'animation frame' that is set on the animation slider to know in which animation frame find the texture
     */
    uint32_t getTextureIdFromItemType(const std::shared_ptr<ItemType>& it, int h, int w, int a) {
        int index = h * it->width + w + ((a-1) * (it->width * it->height));
        return it->textureIdsVector[index];
    }
    /**
     * @brief Main method to set texture in an ItemType
     *
     * Method to set texture in an ItemType at a desired by us position
     * ... so, width, height, animation frame etc.
     *
     * @param it itemType that we want to assign texture id to
     * @param h cell's 'height' to which we want to assign texture id
     * @param w cell's 'width' to which we want to assign texture id
     * @param a cell's 'animation frame' to which we want to assign texture id
     * @param newId textureId that will be set at the desired position. I call it newId, because
     * it substitutes the previously texture id that was at that position (index).
     */
    void setTextureIdFromItemType(std::shared_ptr<ItemType> it, int h, int w, int a, int newId) {
        int index = h * it->width + w + ((a-1) * (it->width * it->height));
        it->textureIdsVector[index] = newId;
    }

    std::shared_ptr<sf::Texture> getPreviewTexture(int itemTypeId);
    void replacePreviewTexture(int itemTypeId, std::shared_ptr<sf::Texture> texture);
    /**
     * @brief Creates preview texture for ItemType
     *
     * This method is very costly, since it copies, makes textures
     * and makes render texture. It should be used with caution.
     *
     * @param id ItemType id
     */
    void createPreviewTexture(int id);
    /**
     * @brief Creates preview texture for all items on page
     *
     * This method is very costly, since it copies, makes textures
     * and makes render texture. It should be used with caution.
     *
     * @param pageFirstItemType Id of first itemType on page
     * @param pageLastItemType Id of last itemType on page
     */
    void createPreviewTexturesForPage(int pageFirstItemType, int pageLastItemType);
    void setDecoyPreviewTexture(int id) {
        replacePreviewTexture(id, std::make_shared<sf::Texture>());
    }
    void clearPreviewTextures();

    /**
     * @brief Returns a sprite sheet of an item
     *
     * Example use, is to call this method with animations = 1, which
     * will effectively help us with creating a preview texture for an itemtype.
     * Another example, is animations = max item's animations, to get full sprite sheet
     * used in exporting the item's png etc.
     *
     * @param itemTypeId id of itemType that we will be creating sprite sheet based on
     * @param animations of how many item's animation frames should the sprite sheet be composed of
     * @return sf::Texture that is composed of however many animation frames were requested in animations param
     */
    sf::Texture getItemSpriteSheet(int itemTypeId, int animations);

    // Helper methods, it is a trick used
    // When we have unsaved item, we click other Item go to it,
    // but we are stopped by the prompt. If we resume, normally
    // we would have to re-select, with this instead, we don't have to.
    [[nodiscard]] int getLastSelectedItemId() const {
        return lastSelectedItemId;
    }
    void setLastSelectedItemId(int id) {
        lastSelectedItemId = id;
    }

    // Helper methods, a trick to know
    // which category was most recently picked
    // Our example use case: if we last clicked itemType, we know
    // we copy to clipboard item. If it was sprites category, a bitmap would be copied into clipboard.
    ASSET_CATEGORY getLastSelectedCategory() {
        return lastSelectedCategory;
    }
    void setLastSelectedCategory(ASSET_CATEGORY category) {
        lastSelectedCategory = category;
    }

    void exportTexture(const std::string& outputString, int textureId);
    void exportTexture(const std::string& outputString, sf::Texture texture);

    void drawAssetsManagerControls();

    GUIHelper* getGuiHelper() {
        return guiHelper;
    }

    [[nodiscard]] bool isGraphicFileLoaded() const {
        return graphicFileLoaded;
    }
    [[nodiscard]] bool isDatFileLoaded() const {
        return datLoaded;
    }
    void setGraphicFileLoaded(bool value) {
        graphicFileLoaded = value;
    }
    void setDatFileLoaded(bool value) {
        datLoaded = value;
    }

    uint32_t getLoadedSprSignature() {
        return loadedSprSignature;
    }
    void setLoadedSprSignature(uint32_t signature) {
        loadedSprSignature = signature;
    }

    std::shared_ptr<sf::Texture> BLANK_TEXTURE;

    const char* const* getVersionsArray() {
        return m_versions;
    }
    int getVersionsArraySize() {
        return static_cast<int>(sizeof(m_versions) / sizeof(m_versions[0]));
    }
    const std::vector<int>& getSpriteDimensionsVector() {
        return m_spriteDimensions;
    }

    // Returns 'true' if 'Compile' button should be available.
    // The main thing is that unless there are changes we shouldn't compile.
    // But there are other cases, such as "new assets", first compilation gotta be "CompileAs".
    // That's why there is 'needPath', that 'Compile' uses, but 'Compile As' doesn't.
    bool isCompilable(bool needPath = true);

    AssetsInfo m_assetsInfo = AssetsInfo();
    AssetsInfo m_tempCreation_AssetsInfo;
private:
    GUIHelper* guiHelper;

    std::vector<std::shared_ptr<sf::Texture>> textures;
    std::vector<std::shared_ptr<sf::Texture>> previewTextures = std::vector<std::shared_ptr<sf::Texture>>();

    // To know the current animation frame slider's value
    int animationFrameSetting = 1;

    bool unsavedSpriteChanges = false;
    bool unsavedItemChanges = false;
    bool unsavedItemTypeChange = false;

    std::shared_ptr<ItemType> unsavedItemTypeCopy;
    int unsavedItemTypeId = -1;

    int lastSelectedItemId = -1; // Useful when we e.g. have unsaved changes and clicked select on some item.
    ASSET_CATEGORY lastSelectedCategory = CATEGORY_ITEMS;

    bool graphicFileLoaded = false; // either .spr or .assets loaded
    bool datLoaded = false; // .dat loaded

    uint32_t loadedSprSignature = 0;

    void buttonLoadGraphics(std::string& foundGraphicFilePath);

    void doPopupAssetFileOpen();
    void doPopupNewAssetFiles();
    void doPopupAssetsCompileAs();

    inline static const char* m_versions[] = {"0.01"};
    inline static std::vector<int> m_spriteDimensions = {32};
};