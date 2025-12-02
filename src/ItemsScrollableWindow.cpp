#include <iostream>
#include <map>
#include "ItemsScrollableWindow.h"
#include "Misc/tools.h"
#include "Things/Item.h"
#include "Misc/definitions.h"
#include "Things/ThingCategory.h"

ItemsScrollableWindow::ItemsScrollableWindow(AssetsManager* am)
: ThingScrollableWindow(am, ThingCategory::ITEM)
{
    // Clear search text field, there were weird '??' artifacts sometimes
    idInputBuffer[0] = '\0';
}

void ItemsScrollableWindow::selectItem(int id, bool goToSelect) {
    assetsManager->setLastSelectedCategory(CATEGORY_ITEMS);

    // Already selected
    if(getSelectedButtonIndex() >= 0 && id == getSelectedButtonIndex()) {
        return;
    }
    // ItemType that we selected doesn't exist
    if(!Items::isValidItemTypeIndex(id)) {
        Warninger::sendWarning(FUNC_NAME, "ItemType that we try to select doesn't exist (" + std::to_string(id) + ")");
        return;
    }

    // Set slider to 1
    // If we go from item that e.g. had 3 animations, to the item that has less than that ...
    assetsManager->setAnimationFrameSetting(1);
    // Stop animation when selecting a different item
    isAnimationPlaying = false;
    // To know what we selected most recently
    assetsManager->setLastSelectedItemId(id);

    if(triggerItemSavePrompt()) {
        return;
    }

    int oldPage = getCurrentPage();
    int newPage = id / ConfigManager::getInstance()->getButtonsCountItemPage();
    setCurrentPage(newPage);
    
    // If page changed, trigger preview generation
    if (oldPage != newPage) {
        onPageChanged(oldPage, newPage, false);
    }

    if(goToSelect) {
        scrollToButtonIndex = id;
    }
    selectedItemIndex = id;
}

int ItemsScrollableWindow::addItemType() {
    if(triggerItemSavePrompt()) {
        return -1;
    }

    auto newItemType = std::make_shared<ItemType>();
    Items::pushItemType(newItemType);
    return getTotalButtons();
}

bool ItemsScrollableWindow::removeItemType() {
    int lastItemTypeId = getTotalButtons() - 1;
    int selectedIndex = getSelectedButtonIndex();
    bool deletingLast = lastItemTypeId == selectedIndex;

    if(selectedIndex == 0) {
        return false; // Let's disable removing itemType index 0
    }
    // Shouldn't happen, but we can check anyway.
    if(selectedIndex < 0 || selectedIndex > lastItemTypeId) {
        Warninger::sendWarning(FUNC_NAME, "Trying to remove ItemType, that does not exist (?) Id: " + std::to_string(selectedIndex));
        return false;
    }
    // This one below shouldn't happen, because it should be already checked while selecting another itemType.
    if(assetsManager->hasUnsavedChanges(CATEGORY_ITEMS_ITEMTYPE)) {
        if(assetsManager->getUnsavedItemTypeId() != getSelectedButtonIndex()) {
            Warninger::sendWarning(FUNC_NAME, "Shouldn't be appearing!" );
            shouldOpenUnsavedPopup = true;
        }
        return false;
    }
    // End of things that shouldn't happen.

    if(deletingLast) {
        Items::removeItemType(selectedIndex);
        setSelectedButtonIndex(getTotalButtons() - 1);
    } else {
        // Deleting for example something in the middle, then we only want to reset preview, attributes etc.
        auto newIt = std::make_shared<ItemType>();
        if(assetsManager->getUnsavedItemTypeId() != -1) {
            assetsManager->setUnsavedItemType(newIt, assetsManager->getUnsavedItemTypeId());
            assetsManager->setDecoyPreviewTexture(assetsManager->getUnsavedItemTypeId());
        } else {
            Warninger::sendWarning(FUNC_NAME, "No unsaved itemType detected, while removing: " + std::to_string(selectedIndex));
        }
    }

    assetsManager->setUnsavedChanges(CATEGORY_ITEMS, true);
    return true;
}

void ItemsScrollableWindow::onPostItemImport() {
    int newestItemIndex = Items::getItemTypesCount()-1;
    assetsManager->createPreviewTexture(newestItemIndex, ThingCategory::ITEM);
    selectItem(newestItemIndex, true);
}

void ItemsScrollableWindow::drawItemTypeList(sf::Clock& deltaClock) {
    // --- Left Panel: Scrollable list of textures ---
    ImGui::BeginGroup();

    ImGui::Text("Items list (Max itemType: %d)", (Items::getItemTypesCount() - 1));

    // Use available space instead of fixed size
    float availableHeight = ImGui::GetContentRegionAvail().y - 60.0f; // Reserve space for controls
    ImVec2 listSize(0, availableHeight > 0 ? availableHeight : 500);
    ImGui::BeginChild("ItemsList", listSize, true);
    if(!assetsManager->isGraphicFileLoaded() || !assetsManager->isDatFileLoaded()) {
        ImGui::Text("Need to load .dat and .spr!");
        ImGui::EndChild();
        ImGui::EndGroup();
        return;
    }

    // Calculate indices for the current page
    int startIndex = getPageFirstIndex();
    int endIndex = getPageLastIndex();

    for (int i = startIndex; i < endIndex; ++i) {
        bool isSelected = (i == getSelectedButtonIndex());
        auto texture = assetsManager->getPreviewTexture(i);

        ImGui::PushID(i);
        if (ImGui::ImageButton
        (
            "##ItemTypeButton",
            (ImTextureID)(uintptr_t)texture->getNativeHandle(),
            ConfigManager::getInstance()->getItemButtonSize(),
            ImVec2(0, 0), ImVec2(1, 1)
        ))
        {
            selectItem(i, false);
        }
        ImGui::PopID();

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            rightMenuClickedItem = i;
            ImGui::OpenPopup("RightClickItemTypeMenu");
        }

        if (rightMenuClickedItem == i && ImGui::BeginPopup("RightClickItemTypeMenu"))
        {
            if (ImGui::MenuItem("Replace"))
            {

            }
            if (ImGui::MenuItem("Export"))
            {

            }
            if (ImGui::MenuItem("Duplicate"))
            {

            }
            if (ImGui::MenuItem("Remove"))
            {

            }

            ImGui::EndPopup();
        }

        // Draw a color border if this texture is selected
        if (isSelected) {
            ImVec2 buttonPos = ImGui::GetItemRectMin();
            ImVec2 buttonSize = ImGui::GetItemRectSize();
            ImU32 borderColor = ConfigManager::getInstance()->getImGuiSelectedThingColor();

            ImGui::GetWindowDrawList()->AddRect(
                buttonPos,
                ImVec2(buttonPos.x + buttonSize.x, buttonPos.y + buttonSize.y),
                borderColor,
                0.0f,
                ImDrawFlags_None,
                3.0f
            );
        }

        if(isSelected && scrollToButtonIndex == i) {
            ImGui::SetScrollHereY();
            scrollToButtonIndex = -1;
        }

        // Display the ID next to the button
        ImGui::SameLine();
        ImGui::Text("ID: %d", i);
    }

    ImGui::EndChild();
    ImGui::EndGroup();
}

void ItemsScrollableWindow::drawPaginationControls() {
    if(!assetsManager->isDatFileLoaded()) {
        ImGui::BeginDisabled();
    }

    ImGui::BeginGroup();

    int startIndex = getPageFirstIndex();
    int endIndex = getPageLastIndex();

    // Pagination controls - first row (using base class helper)
    drawPaginationRow(startIndex, endIndex, "Item", "Item Id");

    ImGui::Spacing();

    // Action buttons - second row (extended with Export/Import)
    ImGui::BeginGroup();
    if (ImGui::Button("New Item##NewItemTypeFromList")) {
        int index = addItemType();
        if (index >= 1) {
            selectItem(index - 1);
            assetsManager->setUnsavedChanges(CATEGORY_ITEMS, true);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove Item##RemoveItemTypeFromList")) {
        bool success = removeItemType();
        if (success) {
            assetsManager->setUnsavedChanges(CATEGORY_ITEMS, true);
        }
    }
    ImGui::SameLine();
    drawGUIItemTypeExport();
    ImGui::SameLine();
    if (ImGui::Button("Import###ImportItemTypeButton")) {
        handleItemTypeImport();
    }
    ImGui::EndGroup();

    ImGui::EndGroup();

    if(!assetsManager->isDatFileLoaded()) {
        ImGui::EndDisabled();
    }
}

bool ItemsScrollableWindow::onPageChange() {
    if(triggerItemSavePrompt()) {
        return false;
    }
    return true;
}

void ItemsScrollableWindow::exportItem(Tools::EXPORT_OPTIONS option) {
    if(!isAnyButtonSelected()) {
        return;
    }
    std::string filePath = (std::filesystem::path(outputFolder) / (Tools::trim(itemName))).string() + Tools::getFormatString(option);

    auto item = Items::getItemType(getSelectedButtonIndex());

    switch(option) {
        case Tools::PNG:
        case Tools::BMP:
        case Tools::JPG:
            assetsManager->exportTexture(filePath, assetsManager->getItemSpriteSheet(getSelectedButtonIndex(), item->animationsFrames));
            break;
        case Tools::TOML:
            Items::exportItemToml(filePath, getSelectedButtonIndex());
            break;
        case Tools::ITF:
            Items::exportItemItf(filePath, getSelectedButtonIndex());
            break;
        default:
            Items::exportItemItf(filePath, getSelectedButtonIndex());
            break;
    }
}

void ItemsScrollableWindow::setSelectedButtonIndex(int id, bool goToSelect) {
    if (id < 0 || id > getTotalButtons()) {
        return;
    }

    int oldPage = getCurrentPage();
    int newPage = id / ConfigManager::getInstance()->getButtonsCountItemPage();
    setCurrentPage(newPage);
    
    // If page changed, trigger preview generation
    // Don't auto-select first item since we're already selecting a specific item
    if (oldPage != newPage) {
        onPageChanged(oldPage, newPage, false);
    }

    if(goToSelect) {
        scrollToButtonIndex = id;
    }
    selectedItemIndex = id;
}

void ItemsScrollableWindow::drawGUIItemTypeExport() {
    if (ImGui::Button("Export###ExportItemTypeButton")) {
        if(isAnyButtonSelected()) {
            itemName = "item" + std::to_string(getSelectedButtonIndex());
            ImGui::OpenPopup("Export Item Popup");
        }
    }
    if(!isAnyButtonSelected()) {
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Please select an item first!");
        }
    }

    if (ImGui::BeginPopupModal("Export Item Popup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Name:");
        ImGui::InputText("##name", &itemName[0], itemName.size() + 1);

        ImGui::Text("Output Folder:");
        if (!Tools::isValidFolderPath(outputFolder)) {
            ImGui::SameLine();
            ImGui::Text("Invalid path!");
        }
        ImGui::PushItemWidth(200);
        ImGui::InputText("##folder", &outputFolder[0], outputFolder.size() + 1);
        ImGui::SameLine();
        if (ImGui::Button("Browse")) {
            auto selectedFolder = Tools::openFileDialogChooseFolder();
            if (!selectedFolder.empty()) {
                outputFolder = selectedFolder;
            }
        }
        ImGui::PopItemWidth();

        ImGui::Text("Format:");
        ImGui::RadioButton("PNG", &exportFormatSelected, 0);
        ImGui::SameLine();
        ImGui::RadioButton("BMP", &exportFormatSelected, 1);
        ImGui::SameLine();
        ImGui::RadioButton("JPG", &exportFormatSelected, 2);
        ImGui::SameLine();
        ImGui::RadioButton("ITF", &exportFormatSelected, 3);
        ImGui::SameLine();
        ImGui::RadioButton("TOML", &exportFormatSelected, 4);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        auto colorsCount = Tools::pushImGuiGray(!Tools::isValidFolderPath(outputFolder));
        if (ImGui::Button("Confirm")) {
            if (Tools::isValidFolderPath(outputFolder)) {
                exportItem(static_cast<Tools::EXPORT_OPTIONS>(exportFormatSelected));
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

        ImGui::EndPopup();
    }
}

void ItemsScrollableWindow::handleItemTypeImport() {
    auto fileChosen = Tools::openFileDialog({"itf", "toml"});

    if (fileChosen.empty()) {
        return; // User canceled the file selection
    }

    // Extract file extension
    std::filesystem::path filePath(fileChosen);
    std::string extension = filePath.extension().string(); // Includes the dot, e.g., ".itf"

    bool successImport = false;
    if (extension == ".itf") {
        successImport = Items::importItemItf(fileChosen);
    }
    else if (extension == ".toml") {
        successImport = Items::importItemToml(fileChosen);
    }
    else {
        Warninger::sendWarning(FUNC_NAME, "Unsupported file format: " + extension);
    }

    if(successImport) {
        onPostItemImport();
    }
}

void ItemsScrollableWindow::drawItemTypePanel() {
    // --- Center Panel: Tabs for Texture and Item Info ---
    ImGui::BeginGroup();
    ImVec2 propertiesGroupSize = ImGui::GetItemRectSize();

    ImGui::Text("Properties List");

    // Use available space instead of fixed size
    float availableHeight = ImGui::GetContentRegionAvail().y;
    ImVec2 panelSize(0, availableHeight > 0 ? availableHeight : 500);
    ImGui::BeginChild("PropertiesPanel", panelSize, true);
    if(!assetsManager->isGraphicFileLoaded() || !assetsManager->isDatFileLoaded()) {
        ImGui::Text("Need to load .dat and .spr!");
        ImGui::EndChild();
        ImGui::EndGroup();
        return;
    }

    if (ImGui::BeginTabBar("Tabs")) {

        // --- Texture Tab ---
        // **Store a copy of the current item state**
        if (getSelectedButtonIndex() >= 0 && getSelectedButtonIndex() < (int)Items::getItemTypesCount()) {
            if (assetsManager->getUnsavedItemTypeId() == -1 ||
                assetsManager->getUnsavedItemTypeId() != getSelectedButtonIndex())
            {
                assetsManager->setUnsavedItemType(Items::getItemType(getSelectedButtonIndex()), getSelectedButtonIndex());
            }
        }

        std::shared_ptr<ItemType> previewIt = assetsManager->getUnsavedItemType();
        if (ImGui::BeginTabItem("Texture")) {
            if (getSelectedButtonIndex() >= 0 && getSelectedButtonIndex() < (int)Items::getItemTypesCount()) {
                ImGui::BeginGroup();
                ImVec2 groupSize = ImGui::GetContentRegionAvail();

                ImGui::Text("Texture Preview (Drop sprites here)");
                auto oldPos = ImGui::GetCursorPos();
                auto spriteMaxSize = ConfigManager::getInstance()->getSpriteMaxSize();

                // Calculate total preview dimensions including all patterns
                // Layout: patternZ * patternX columns horizontally, patternY rows vertically
                // Each cell is (width * spriteMaxSize) x (height * spriteMaxSize)
                int totalColumnsX = previewIt->patternZ * previewIt->patternX;
                int totalRowsY = previewIt->patternY;
                int cellPixelWidth = previewIt->width * spriteMaxSize;
                int cellPixelHeight = previewIt->height * spriteMaxSize;
                int totalPreviewWidth = totalColumnsX * cellPixelWidth;
                int totalPreviewHeight = totalRowsY * cellPixelHeight;

                ImVec2 centeredPos(groupSize.x / 2.0f - (static_cast<float>(totalPreviewWidth) / 2.0f), oldPos.y);
                ImVec2 gridPos = centeredPos;

                // Clear the preview area first by drawing a background
                ImVec2 previewAreaSize(static_cast<float>(totalPreviewWidth), static_cast<float>(totalPreviewHeight));
                ImVec2 previewAreaMin = centeredPos;
                ImVec2 previewAreaMax(previewAreaMin.x + previewAreaSize.x, previewAreaMin.y + previewAreaSize.y);
                
                // Draw transparent/black background to clear previous sprites
                ImGui::GetWindowDrawList()->AddRectFilled(previewAreaMin, previewAreaMax, IM_COL32(0, 0, 0, 0));

                // Loop through all patterns and display them in a grid
                // Rows: patternY (top to bottom)
                // Columns: patternZ * patternX (left to right, Z varies slowest, X varies fastest)
                for (int pY = 0; pY < previewIt->patternY; pY++) {
                    for (int pZ = 0; pZ < previewIt->patternZ; pZ++) {
                        for (int pX = 0; pX < previewIt->patternX; pX++) {
                            // Calculate column index: pZ * patternX + pX
                            int colIdx = pZ * previewIt->patternX + pX;
                            
                            // Draw all sprites (width x height) for this pattern cell
                            for (int l = 0; l < previewIt->layers; l++) {
                                for (int w = 0; w < previewIt->width; w++) {
                                    for (int h = 0; h < previewIt->height; h++) {
                                        int spriteIndex = assetsManager->getTextureIdFromItemType(
                                            previewIt, w, h, assetsManager->getAnimationFrameSetting(), 
                                            l, pX, pY, pZ);
                                        auto texture = assetsManager->getTexture(spriteIndex);

                                        if (texture) {
                                            ImVec2 previewSize = ImVec2((float)texture->getSize().x, (float)texture->getSize().y);

                                            // Calculate position:
                                            // Base position + column offset + cell-internal offset (flipped)
                                            auto tempPos = centeredPos;
                                            tempPos.x += static_cast<float>(colIdx * cellPixelWidth);
                                            tempPos.y += static_cast<float>(pY * cellPixelHeight);
                                            // Flipped positioning within the cell
                                            tempPos.x += std::floor((float)(previewIt->width - w - 1) * spriteMaxSize);
                                            tempPos.y += std::floor((float)(previewIt->height - h - 1) * spriteMaxSize);
                                            
                                            ImGui::SetCursorPos(tempPos);
                                            ImGui::Image((ImTextureID)(uintptr_t)texture->getNativeHandle(), previewSize);

                                            if (ImGui::IsItemHovered()) {
                                                ImGui::SetTooltip("Sprite: %d\nPos: w=%d, h=%d, l=%d\nPattern: X=%d, Y=%d, Z=%d", 
                                                    spriteIndex, w, h, l, pX, pY, pZ);
                                            }

                                            // Handle drag-and-drop target
                                            if (ImGui::BeginDragDropTarget()) {
                                                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("TEXTURE_ID")) {
                                                    int newTextureId = *(int *) payload->Data;
                                                    assetsManager->setTextureIdFromItemType(
                                                        previewIt, w, h, assetsManager->getAnimationFrameSetting(), 
                                                        newTextureId, l, pX, pY, pZ);
                                                    // Force preview update
                                                    assetsManager->createPreviewTexture(getSelectedButtonIndex(), ThingCategory::ITEM);
                                                }
                                                ImGui::EndDragDropTarget();
                                            }

                                            // Set grid position from the top-left corner (first pattern cell, first sprite)
                                            if (pX == 0 && pY == 0 && pZ == 0 && 
                                                w == previewIt->width - 1 && h == previewIt->height - 1 && l == 0) {
                                                gridPos = ImGui::GetItemRectMin();
                                            }
                                        } else {
                                            Warninger::sendWarning(FUNC_NAME, "No texture detected while displaying item's textures");
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Draw Grid Checkbox
                float currentY = ImGui::GetCursorPosY();
                ImGui::SetCursorPosY(currentY + 30);
                ImGui::Checkbox("Draw Grid", &drawGrid);

                // Animation Frame Slider
                ImGui::SameLine();
                float width = ImGui::GetContentRegionAvail().x * 0.15f;
                ImGui::PushItemWidth(width);
                int previousFrame = assetsManager->getAnimationFrameSetting();
                ImGui::SliderInt("Animation Frame", &assetsManager->getAnimationFrameSettingRef(), 1, previewIt->animationsFrames);
                // Stop animation if user manually changes the slider
                if (assetsManager->getAnimationFrameSetting() != previousFrame) {
                    isAnimationPlaying = false;
                }
                ImGui::PopItemWidth();

                // Play/Pause Animation Button
                ImGui::SameLine();
                const char* playButtonLabel = isAnimationPlaying ? "||" : ">";
                bool canAnimate = previewIt->animationsFrames > 1;
                if (!canAnimate) {
                    isAnimationPlaying = false; // Stop if item has only 1 frame
                }
                if (ImGui::Button(playButtonLabel, ImVec2(30, 0))) {
                    if (canAnimate) {
                        isAnimationPlaying = !isAnimationPlaying;
                        if (isAnimationPlaying) {
                            animationClock.restart();
                        }
                    }
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(isAnimationPlaying ? "Pause Animation" : "Play Animation");
                }

                // Animation loop logic
                if (isAnimationPlaying && canAnimate) {
                    float elapsed = animationClock.getElapsedTime().asSeconds();
                    if (elapsed >= ConfigManager::getInstance()->getAnimationFrameTime()) {
                        // Advance to next frame
                        int currentFrame = assetsManager->getAnimationFrameSetting();
                        int nextFrame = currentFrame + 1;
                        // Loop using modulo: frame 1 to animationsFrames, then back to 1
                        if (nextFrame > previewIt->animationsFrames) {
                            nextFrame = 1;
                        }
                        assetsManager->setAnimationFrameSetting(nextFrame);
                        animationClock.restart();
                    }
                }

                //ImVec2 gridPos = ImGui::GetItemRectMin(); // ImGui::GetItemRectMin()
                ImVec2 gridTotalSize = ImVec2(static_cast<float>(totalPreviewWidth), static_cast<float>(totalPreviewHeight));
                if (drawGrid) {
                    for (int x = 0; x <= gridTotalSize.x; x += spriteMaxSize) {
                        ImGui::GetWindowDrawList()->AddLine(
                                ImVec2(gridPos.x + x, gridPos.y),
                                ImVec2(gridPos.x + x, gridPos.y + gridTotalSize.y),
                                ConfigManager::getInstance()->getImGuiGridColor(),
                                1.0f
                        );
                    }
                    // Draw horizontal grid lines
                    for (int y = 0; y <= gridTotalSize.y; y += spriteMaxSize) {
                        ImGui::GetWindowDrawList()->AddLine(
                                ImVec2(gridPos.x, gridPos.y + y),
                                ImVec2(gridPos.x + gridTotalSize.x, gridPos.y + y),
                                ConfigManager::getInstance()->getImGuiGridColor(),
                                1.0f
                        );
                    }
                }

                ImGui::NewLine();
                ImGui::Separator();

                // Set fixed Y-position for sliders
                ImGui::SetCursorPosY(oldPos.y + groupSize.y/2 + 10);
                //ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);

                ItemType tempCopyIt = *previewIt;
                if (ImGui::BeginTable("ItemTextureProperties", 3, ImGuiTableFlags_SizingFixedFit)) {
                    // Set up columns
                    ImGui::TableSetupColumn("Column 1", ImGuiTableColumnFlags_WidthFixed, (groupSize.x/2));
                    ImGui::TableSetupColumn("Column 2", ImGuiTableColumnFlags_WidthFixed, (groupSize.x/4));
                    ImGui::TableSetupColumn("Column 3", ImGuiTableColumnFlags_WidthFixed, groupSize.x/4);
                    ImGui::TableNextColumn();

                    ImGui::TableNextColumn();
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Width:");
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Height:");
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Layers:");
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Pattern X:");
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Pattern Y:");
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Pattern Z:");
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Animations:");

                    ImGui::TableNextColumn();
                    ImGui::PushItemWidth(groupSize.x * 0.20f);

                    int width = tempCopyIt.width;
                    if (ImGui::InputInt("##Width", &width, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        width = std::clamp(width, 1, ConfigManager::getInstance()->getItemMaxWidth());
                        tempCopyIt.width = static_cast<uint8_t>(width);
                        previewIt->setItemTypeWidth(tempCopyIt.width);
                    }

                    int height = tempCopyIt.height;
                    if (ImGui::InputInt("##Height", &height, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        height = std::clamp(height, 1, ConfigManager::getInstance()->getItemMaxHeight());
                        tempCopyIt.height = static_cast<uint8_t>(height);
                        previewIt->setItemTypeHeight(tempCopyIt.height);
                    }

                    int layers = tempCopyIt.layers;
                    if (ImGui::InputInt("##Layers", &layers, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        layers = std::clamp(layers, 1, 10); // Reasonable max for layers
                        tempCopyIt.layers = static_cast<uint8_t>(layers);
                        previewIt->setItemTypeLayers(tempCopyIt.layers);
                    }

                    int patternX = tempCopyIt.patternX;
                    if (ImGui::InputInt("##PatternX", &patternX, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        patternX = std::clamp(patternX, 1, 10); // Reasonable max for patternX
                        tempCopyIt.patternX = static_cast<uint8_t>(patternX);
                        previewIt->setItemTypePatternX(tempCopyIt.patternX);
                    }

                    int patternY = tempCopyIt.patternY;
                    if (ImGui::InputInt("##PatternY", &patternY, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        patternY = std::clamp(patternY, 1, 10); // Reasonable max for patternY
                        tempCopyIt.patternY = static_cast<uint8_t>(patternY);
                        previewIt->setItemTypePatternY(tempCopyIt.patternY);
                    }

                    int patternZ = tempCopyIt.patternZ;
                    if (ImGui::InputInt("##PatternZ", &patternZ, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        patternZ = std::clamp(patternZ, 1, 10); // Reasonable max for patternZ
                        tempCopyIt.patternZ = static_cast<uint8_t>(patternZ);
                        previewIt->setItemTypePatternZ(tempCopyIt.patternZ);
                    }

                    int animations = tempCopyIt.animationsFrames;
                    if (ImGui::InputInt("##Animations", &animations, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        animations = std::clamp(animations, 1, ConfigManager::getInstance()->getItemMaxAnimationCount());
                        tempCopyIt.animationsFrames = static_cast<uint8_t>(animations);
                        previewIt->setItemTypeAnimationCount(tempCopyIt.animationsFrames);

                        if(tempCopyIt.animationsFrames < assetsManager->getAnimationFrameSetting()) {
                            assetsManager->setAnimationFrameSetting(tempCopyIt.animationsFrames);
                        }
                    }

                    ImGui::PopItemWidth();
                    ImGui::EndTable();
                }

                ImGui::EndGroup();
            } else {
                ImGui::Text("No texture selected");
                // Stop animation if no item is selected
                isAnimationPlaying = false;
            }
            ImGui::EndTabItem();
        } else {
            // Stop animation when not on texture tab
            isAnimationPlaying = false;
        }

        // --- Item Info Tab ---
        if (ImGui::BeginTabItem("Properties")) {
            drawLightControlSegment(*previewIt);

            // --- Minimap Color Section ---
            if (ImGui::CollapsingHeader("Minimap Color", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Selected Color:");
                ImGui::SameLine();

                int colorValue = previewIt->minimapColor;
                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::InputInt("##MinimapColorInt", &colorValue, 1, 5)) {
                    colorValue = std::clamp(colorValue, 0, 215);
                    previewIt->minimapColor = colorValue;
                }

                // Compute current RGB
                int cr = ((previewIt->minimapColor / 36) % 6) * 51;
                int cg = ((previewIt->minimapColor / 6) % 6) * 51;
                int cb = (previewIt->minimapColor % 6) * 51;

                ImVec4 currentColor = ImVec4(cr / 255.0f, cg / 255.0f, cb / 255.0f, 1.0f);

                ImGui::SameLine();

                // The trigger button
                if (ImGui::ColorButton("##CurrentMinimapColor", currentColor,
                    ImGuiColorEditFlags_NoTooltip, ImVec2(30, 30)))
                {
                    ImGui::OpenPopup("MinimapColorPicker");
                }

                // Popup with color grid
                if (ImGui::BeginPopup("MinimapColorPicker"))
                {
                    const int colorsPerRow = 16;
                    const int totalColors = 216;
                    const float buttonSize = 20.0f;
                    const float spacing = 2.0f;

                    ImGui::Text("Select Color:");
                    ImGui::Separator();

                    for (int idx = 0; idx < totalColors; ++idx)
                    {
                        int r = ((idx / 36) % 6) * 51;
                        int g = ((idx / 6) % 6) * 51;
                        int b = (idx % 6) * 51;
                        ImVec4 c = ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);

                        ImGui::PushID(idx);

                        if (ImGui::ColorButton("##pick", c,
                            ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
                            ImVec2(buttonSize, buttonSize)))
                        {
                            previewIt->minimapColor = idx;
                            ImGui::CloseCurrentPopup();
                        }

                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("ID: %d\nR: %d\nG: %d\nB: %d", idx, r, g, b);
                        }

                        // highlight selected color
                        if (previewIt->minimapColor == idx)
                        {
                            ImVec2 min = ImGui::GetItemRectMin();
                            ImVec2 max = ImGui::GetItemRectMax();
                            ImGui::GetWindowDrawList()->AddRect(
                                min, max, IM_COL32(255,255,0,255), 0.0f, 0, 2.0f);
                        }

                        ImGui::PopID();

                        if ((idx + 1) % colorsPerRow != 0)
                            ImGui::SameLine(0, spacing);
                    }

                    ImGui::EndPopup();
                }

                ImGui::NewLine();
            }

            // Load and display properties checkboxes
            if (ImGui::CollapsingHeader("Flags", ImGuiTreeNodeFlags_DefaultOpen)) {
                // Radio button group for mutually exclusive options
                ImGui::Text("Flag Type:");
                if (ImGui::RadioButton("Common", previewIt->itemCategory == COMMON)) {
                    previewIt->itemCategory = COMMON;
                };
                if (ImGui::RadioButton("Ground Border", previewIt->itemCategory == GROUND_BORDER)) {
                    previewIt->itemCategory = GROUND_BORDER;
                };
                if (ImGui::RadioButton("Bottom", previewIt->itemCategory == BOTTOM)) {
                    previewIt->itemCategory = BOTTOM;
                };
                if (ImGui::RadioButton("Top", previewIt->itemCategory == TOP)) {
                    previewIt->itemCategory = TOP;
                };

                ImGui::Separator(); // A line to separate radio group from checkboxes

                if (ImGui::BeginTable("BooleanItemProperties", 2)) {
                    ImGui::TableNextColumn();

                    const std::vector<std::pair<const char*, ItemTypeFlags>> flagNames = {
                            {"Ground", IS_GROUND},
                            {"Container", IS_CONTAINER},
                            {"Stackable", STACKABLE},
                            {"Unpassable", UNPASSABLE},
                            {"Unmovable", UNMOVABLE},
                            {"Force Use", FORCE_USE},
                            {"MultiUse", MULTI_USE},
                            {"Pickupable", PICKUPABLE},
                            {"Block Missile", BLOCK_MISSILE}
                    };
                    for (size_t i = 0; i < flagNames.size(); ++i) {
                        if (i > 0 && i % (flagNames.size() / 2) == 0) { // Move to the next column on half
                            ImGui::TableNextColumn();
                        }

                        bool checked = previewIt->hasFlag(flagNames[i].second);
                        if (ImGui::Checkbox(flagNames[i].first, &checked)) {
                            previewIt->setFlag(flagNames[i].second, checked);
                        }
                    }

                    ImGui::EndTable();
                }

                ImGui::NewLine();
            }

            ImGui::EndTabItem();
        }

        // **Compare new and old state to check for changes**
        if (getSelectedButtonIndex() >= 0 && getSelectedButtonIndex() < (int)Items::getItemTypesCount()) {
            if (assetsManager->getUnsavedItemTypeId() != -1 &&
                assetsManager->getUnsavedItemTypeId() == getSelectedButtonIndex()) {
                if (*assetsManager->getUnsavedItemType() != *Items::getItemType(getSelectedButtonIndex())) {
                    assetsManager->setUnsavedChanges(CATEGORY_ITEMS_ITEMTYPE, true);
                } else {
                    assetsManager->setUnsavedChanges(CATEGORY_ITEMS_ITEMTYPE, false);
                }
            } else {
                Warninger::sendWarning(FUNC_NAME, "Current UnsavedItemTypeid is not the same as selection. This warning shouldn't happen.");
            }
        }

        ImGui::EndTabBar();
    }

    // ------- Save Item Button - positioned at bottom center
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Center the save button
    float buttonWidth = 120.0f;
    float availableWidth = ImGui::GetContentRegionAvail().x;
    float offset = (availableWidth - buttonWidth) * 0.5f;
    if (offset > 0)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
    
    auto colorsCount = Tools::pushImGuiGray(!assetsManager->hasUnsavedChanges(CATEGORY_ITEMS_ITEMTYPE));
    if (ImGui::Button("Save Item", ImVec2(buttonWidth, 0))) {
        triggerItemSavePrompt();
    }
    ImGui::PopStyleColor(colorsCount);

    ImGui::EndChild();

    if (shouldOpenUnsavedPopup && assetsManager->getUnsavedItemTypeId() != -1) {
        ImGui::OpenPopup("Unsaved Item Changes");
        shouldOpenUnsavedPopup = false;  // <-- Reset flag after opening popup
    }

    if (ImGui::BeginPopupModal("Unsaved Item Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        std::string text = std::string("Save Changes in the Item ") + std::to_string(assetsManager->getUnsavedItemTypeId()) + "?";
        ImGui::Text("%s", text.c_str());

        if (ImGui::Button("Yes")) {
            assetsManager->setUnsavedChanges(CATEGORY_ITEMS, true);
            assetsManager->setUnsavedChanges(CATEGORY_ITEMS_ITEMTYPE, false);

            auto replacementItem = std::make_shared<ItemType>(*assetsManager->getUnsavedItemType());
            bool replaceSuccess = Items::replaceItemType(assetsManager->getUnsavedItemTypeId(), replacementItem);

            assetsManager->createPreviewTexture(getSelectedButtonIndex(), ThingCategory::ITEM);
            assetsManager->resetUnsavedItemType();

            selectItem(assetsManager->getLastSelectedItemId(), true);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No")) {
            assetsManager->setUnsavedChanges(CATEGORY_ITEMS_ITEMTYPE, false);

            assetsManager->createPreviewTexture(getSelectedButtonIndex(), ThingCategory::ITEM);
            assetsManager->resetUnsavedItemType();

            selectItem(assetsManager->getLastSelectedItemId(), true);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::EndGroup();
}