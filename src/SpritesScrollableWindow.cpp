#include <iostream>
#include "SpritesScrollableWindow.h"
#include "Misc/tools.h"
#include "Helper/SavedData.h"
#include "Misc/definitions.h"

SpritesScrollableWindow::SpritesScrollableWindow(sf::RenderWindow& window, AssetsManager* am)
: window(window)
{
    assetsManager = am;

    // Clear search text field, there were weird '??' artifacts sometimes
    idInputBuffer[0] = '\0';
}

void SpritesScrollableWindow::selectSprite(int id, bool goToSelect) {
    assetsManager->setLastSelectedCategory(CATEGORY_SPRITES);

    // Already selected
    if(getSelectedSpriteIndex() >= 0 && id == getSelectedSpriteIndex()) {
        return;
    }
    // Texture that we selected doesn't exist
    if(!assetsManager->isValidTextureIndex(id)) {
        Warninger::sendWarning(FUNC_NAME, "Texture that we try to select doesn't exist (" + std::to_string(id) + ")");
        return;
    }

    setSelectedButtonIndex(id);
}

void SpritesScrollableWindow::drawTextureList(sf::Clock& deltaClock) {
    // --- Right Panel: Scrollable list of textures ---
    ImGui::BeginGroup();

    ImGui::Text("Sprites list (Max spr: %d)", (assetsManager->getTextureCount() - 1));

    // Create a scrollable panel
    ImVec2 listSize(250, 500);
    ImGui::BeginChild("ScrollablePanel", listSize, true, ImGuiWindowFlags_HorizontalScrollbar);
    if(!assetsManager->isGraphicFileLoaded()) {
        ImGui::Text("Need to load .spr!");
        ImGui::EndChild();
        ImGui::EndGroup();
        return;
    }

    // Calculate the starting and ending indices for the current page
    int startIndex = getPageFirstIndex();
    int endIndex = getPageLastIndex();

    // Create buttons in the scrollable panel for the current page
    for (int i = startIndex; i < endIndex; ++i) {
        bool isSelected = (i == getSelectedSpriteIndex());
        auto texture = assetsManager->getTexture(i);

        ImGui::PushID(i);
        if (ImGui::ImageButton
        (
            "##SpriteButton",
            (ImTextureID)texture->getNativeHandle(),
            ConfigManager::getInstance()->getSpriteButtonSize(), // size
            ImVec2(0, 0), // uv0, default to (0,0) for full texture
            ImVec2(1, 1) // uv1, default to (1,1) for full texture
        ))
        {
            selectSprite(isSelected ? -1 : i, false);
        }
        ImGui::PopID();

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            rightMenuClickedSprite = i;
            ImGui::OpenPopup("RightClickTextureMenu");
        }
        if (rightMenuClickedSprite == i && ImGui::BeginPopup("RightClickTextureMenu"))
        {
            if (ImGui::MenuItem("Copy"))
            {

            }
            if (ImGui::MenuItem("Paste"))
            {

            }
            if (ImGui::MenuItem("Replace"))
            {

            }
            if (ImGui::MenuItem("Export"))
            {

            }
            if (ImGui::MenuItem("Remove"))
            {

            }

            ImGui::EndPopup();
        }

        // Position && Size of the button created
        ImVec2 buttonPos = ImGui::GetItemRectMin();
        ImVec2 buttonSize = ImGui::GetItemRectSize();

        // Draw a color border if this texture is selected
        if (isSelected) {
            ImU32 borderColor = ConfigManager::getInstance()->getImGuiSelectedThingColor();
            ImGui::GetWindowDrawList()->AddRect(
                buttonPos,
                ImVec2(buttonPos.x + buttonSize.x, buttonPos.y + buttonSize.y),
                borderColor,
                0.0f,          // Rounding
                ImDrawFlags_None,
                3.0f
            );
        }

        // Start drag source
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("TEXTURE_ID", &i, sizeof(int)); // Attach the texture ID as payload
            ImGui::Image((ImTextureID)texture->getNativeHandle(), ImVec2(48, 48));
            ImGui::EndDragDropSource();
        }

        // Scroll to the currently selected button
        if(isSelected && scrollToButtonIndex == i) {
            ImGui::SetScrollHereY();
            scrollToButtonIndex = -1;
        }

        // Display ID number next to the button
        ImGui::SameLine();
        ImGui::Text("ID: %d", i);
    }

    ImGui::EndChild();

    // Popup code for invalid selection
    if (ImGui::BeginPopupModal("Error Remove/Replace", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Invalid selection! Please select a valid texture to replace/remove.\nYou can remove only last sprite.\nYou can use replace otherwise.");
        ImGui::Separator();
        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Popup code for invalid tile size on new texture
    if (ImGui::BeginPopupModal("Wrong Texture Size", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        auto spriteMaxSize = ConfigManager::getInstance()->getSpriteMaxSize();
        ImGui::Text(("Invalid image! You tried to import a sprite that is not size: "
                     + std::to_string(spriteMaxSize) + "x"
                     + std::to_string(spriteMaxSize)).c_str());
        ImGui::Separator();
        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::EndGroup();

    // Drag n drop sprites into the scrollable sprites panel
      // Outline on sprites panel when hovered
        // Get bounding box of the group (last item)
    ImVec2 p_min = ImGui::GetItemRectMin();
    ImVec2 p_max = ImGui::GetItemRectMax();
        // Check if mouse is hovering inside that rect
    ImVec2 mouse_pos = ImGui::GetMousePos();
    bool hovered = (mouse_pos.x >= p_min.x && mouse_pos.x <= p_max.x
                    && mouse_pos.y >= p_min.y && mouse_pos.y <= p_max.y);

    if (hovered && !dropManager->GetDraggedFiles().empty()) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        // Draw the golden outline
        draw_list->AddRect(p_min, p_max,  IM_COL32(255, 215, 0, 255), 0.0f, 0, 1.0f);

        if (!dropManager->IsDraggingFiles() && !dropManager->GetDraggedFiles().empty())
        {
            const auto& files = dropManager->GetDraggedFiles();
            const auto& allowedExtensions = Tools::getImageExtensions();

            for (const auto& filePath : files)
            {
                std::string ext = Tools::getExtensionFromPath(filePath);
                if (std::find(allowedExtensions.begin(), allowedExtensions.end(), ext) != allowedExtensions.end()) {
                    importTexture(filePath);
                }
            }

            // TO-DO in the future when you also do the dragging for other things, you will probably need to clear it elsewhere
            dropManager->ClearDraggedFiles();
        }
    }
}

void SpritesScrollableWindow::drawListControlButtons() {
    if(!assetsManager->isGraphicFileLoaded()) {
        ImGui::BeginDisabled();
    }

    ImGui::BeginGroup();

    int startIndex = getPageFirstIndex();
    int endIndex = getPageLastIndex();

    // Pagination controls
    if (ImGui::Button("<< Page##TextureListPageDec")) {
        decrementPage();
    }
    ImGui::SameLine();

    // Show the current range of IDs
    ImGui::Text("Range: %d-%d", startIndex, endIndex - 1);
    ImGui::SameLine();

    // Input field for navigation with a limited width
    ImGui::SetNextItemWidth(50);
    if (ImGui::InputText("Sprite Id##SpriteIdSearchTextField", idInputBuffer, sizeof(idInputBuffer), ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
        int inputId = 0;
        try {
            inputId = std::stoi(idInputBuffer); // Convert input text to integer
        } catch (...) {
            Warninger::sendWarning(FUNC_NAME, "Cannot convert input to a number");
        }
        selectSprite(inputId, false);
    }

    ImGui::SameLine();
    if (ImGui::Button("Page >>##TextureListPageInc")) {
        incrementPage();
    }

    if (ImGui::Button("Replace##ReplaceInTextureList")) {
        if (!assetsManager->isValidTextureIndex(selectedButtonIndex)) {
            ImGui::OpenPopup("Error Remove/Replace");
        } else {
            // Open file dialog to choose a PNG file
            auto filename = Tools::openFileDialog({"png"});

            if (!filename.empty()) {
                auto newTexture = std::make_shared<sf::Texture>();
                if (newTexture->loadFromFile(filename)) {
                    int id = selectedButtonIndex; // Get the ID of the texture you want to replace
                    assetsManager->replaceTexture(id, newTexture); // Replace it with the new texture
                    setUnsavedChanges(true);
                } else {
                    Warninger::sendWarning(FUNC_NAME, "Failed to load texture from: " + filename);
                }
            }
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Import##ImportIntoTextureList")) {
        // Open file dialog to choose a PNG file
        auto filePath = Tools::openFileDialog({"png"});

        if (!filePath.empty()) { // If a file was selected
            importTexture(filePath);
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Export###ExportSpriteButton")) {
        if(isAnySpriteSelected()) {
            spriteName = "sprite" + std::to_string(getSelectedSpriteIndex());
            ImGui::OpenPopup("Export Sprite Popup");
        }
    }
    if(!isAnySpriteSelected()) {
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Please select a sprite first!");
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("New###NewSpriteButton")) {
        createNewTexture();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Adds new sprite at the end of the list");
    }

    ImGui::SameLine();
    if (ImGui::Button("Remove##RemoveInTextureList")) {
        if (selectedButtonIndex != (assetsManager->getTextureCount() - 1)) {
            // If it's not last, we don't allow removal
            ImGui::OpenPopup("Error Remove/Replace");
        } else {
            assetsManager->removeTexture(selectedButtonIndex);
            setUnsavedChanges(true);
            selectedButtonIndex = assetsManager->getTextureCount() - 1;
        }
    }

    if (ImGui::BeginPopupModal("Export Sprite Popup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Name:");
        ImGui::InputText("##name", &spriteName[0], spriteName.size() + 1);

        ImGui::Text("Output Folder:");
        if (!Tools::isValidFolderPath(outputFolder)) {
            ImGui::SameLine();
            ImGui::Text("Invalid path!");
        }
        ImGui::PushItemWidth(200);
        ImGui::InputText("##folder", &outputFolder[0], outputFolder.size() + 1);
        ImGui::SameLine();
        if (ImGui::Button("Browse")) {
            std::string selectedFolder = Tools::openFileDialogChooseFolder();
            if (!selectedFolder.empty()) {
                outputFolder = selectedFolder;
            }
        }
        ImGui::PopItemWidth();

        ImGui::Text("Format:");
        ImGui::RadioButton("PNG", &formatSelected, 0); ImGui::SameLine();
        ImGui::RadioButton("BMP", &formatSelected, 1); ImGui::SameLine();
        ImGui::RadioButton("JPG", &formatSelected, 2);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImVec2 popupSize = ImGui::GetWindowSize();

        auto colorsCount = Tools::pushImGuiGray(!Tools::isValidFolderPath(outputFolder));
        if (ImGui::Button("Confirm")) {
            if (Tools::isValidFolderPath(outputFolder)) {
                exportTexture(static_cast<Tools::EXPORT_OPTIONS>(formatSelected));
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::PopStyleColor(colorsCount);

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            if(!Tools::isValidFolderPath(outputFolder)) {
                outputFolder = Tools::getDesktopPath();
            }
            ImGui::CloseCurrentPopup();
        }

        // Preview of exported sprite
        ImGui::SameLine();
        ImVec2 previewSize = {32, 32};
        float centerPosX = (popupSize.x - previewSize.x - 10);
        ImGui::SetCursorPosX(centerPosX);
        ImGui::Image(assetsManager->getImGuiTexture(getSelectedSpriteIndex()), previewSize);

        ImGui::EndPopup();
    }

    ImGui::EndGroup();

    if(!assetsManager->isGraphicFileLoaded()) {
        ImGui::EndDisabled();
    }
}

void SpritesScrollableWindow::exportTexture(Tools::EXPORT_OPTIONS option) {
    std::string filePath = (std::filesystem::path(outputFolder) / (Tools::trim(spriteName))).string() + getFormatString(option);
    assetsManager->exportTexture(filePath, getSelectedSpriteIndex());
}

void SpritesScrollableWindow::importTexture(const std::string &filePath) {
    if(filePath.empty()) {
        return;
    }

    auto newTexture = std::make_shared<sf::Texture>();
    if (newTexture->loadFromFile(filePath)) {
        bool added = assetsManager->pushTexture(newTexture);
        if(added) {
            int lastTextureIndex = assetsManager->getTextureCount()-1;
            selectSprite(lastTextureIndex, true);
            setUnsavedChanges(true);
        } else {
            ImGui::OpenPopup("Wrong Texture Size");
        }
    } else {
        Warninger::sendWarning(FUNC_NAME, "Failed to load texture from: " + filePath);
    }
}

void SpritesScrollableWindow::createNewTexture() {
    assetsManager->createNewTexture();
}