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
#include "../Things/Outfits.h"
#include "../Things/Effects.h"
#include "../Things/Missiles.h"
#include "../Things/ThingType.h"

AssetsManager::AssetsManager(GUIHelper* guiHelper) 
    : guiHelper(guiHelper), 
      previewManager(&textureManager),
      sprFileHandler(&textureManager, &m_assetsInfo, this),
      datFileHandler(&m_assetsInfo) {
    // TextureManager and PreviewManager are initialized via their constructors
    // BLANK_TEXTURE is now managed by TextureManager
    BLANK_TEXTURE = textureManager.getBlankTexture();

    // Setup Temp Info for "New Assets" creation
    m_tempCreation_AssetsInfo.extended = SavedData::getInstance()->getDataBool("sprExtended");
    m_tempCreation_AssetsInfo.transparency = SavedData::getInstance()->getDataBool("sprTransparency");
}

AssetsManager::~AssetsManager() {
    unload();
}

// getTexture is now inline in header, delegating to textureManager

// Helper function to read a little-endian 16-bit integer from a byte array
uint16_t readLE16(const uint8_t* data) {
    return data[0] | (data[1] << 8);
}

// loadSpr is now inline in header, delegating to sprFileHandler
// Old implementation removed - see SprFileHandler.cpp
// Commented out to avoid compilation errors
/*
bool AssetsManager::loadSpr_OLD(const std::string& sprFilePath) {
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
    textureManager.getTextures().reserve(1 + spriteCount);
    textureManager.getTextures().push_back(textureManager.getBlankTexture());

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
            texture = textureManager.getBlankTexture();
        } else {
            file.seekg(offset, std::ios::beg);
            if (!file.good()) {
                Warninger::sendWarning(FUNC_NAME, "Failed to seek to offset for sprite " + std::to_string(spriteId) + ". Using blank texture.");
                texture = textureManager.getBlankTexture();
            } else {
                file.ignore(3); // Skip unused bytes (RGB)

                // Read sprite data size
                uint16_t dataSize;
                file.read(reinterpret_cast<char*>(&dataSize), 2);
                
                if (!file.good() || dataSize == 0) {
                    // Empty sprite or read error, use blank texture
                    texture = textureManager.getBlankTexture();
                } else {
                    // Read compressed sprite data
                    std::vector<uint8_t> spriteData(dataSize);
                    file.read(reinterpret_cast<char*>(spriteData.data()), dataSize);
                    
                    if (!file.good()) {
                        Warninger::sendWarning(FUNC_NAME, "Failed to read sprite data for sprite " + std::to_string(spriteId) + ". Using blank texture.");
                        texture = textureManager.getBlankTexture();
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
                            texture = textureManager.getBlankTexture();
                        }
                    }
                }
            }
        }
        
        // ALWAYS push a texture (even if blank) to maintain correct sprite ID to texture index mapping
        textureManager.getTextures().push_back(texture);
    }

    onGraphicsLoaded(decidedPath);
    return true;
}
*/

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

void AssetsManager::unloadDat() {
    Items::clearItemTypes();
    Outfits::clearOutfitTypes();
    Effects::clearEffectTypes();
    Missiles::clearMissileTypes();
    clearPreviewTextures();
}

void AssetsManager::unloadTextures() {
    textureManager.clear();
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
    ImGui::InputText("##OutputPath", &m_assetsInfo.outputPath, m_assetsInfo.outputPath.size());
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
