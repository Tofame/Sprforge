#include <iostream>
#include <map>
#include "ItemsScrollableWindow.h"
#include "Misc/tools.h"
#include "Things/Item.h"
#include "Misc/definitions.h"

ItemsScrollableWindow::ItemsScrollableWindow(sf::RenderWindow& window, AssetsManager* am)
: window(window)
{
    assetsManager = am;

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
    // To know what we selected most recently
    assetsManager->setLastSelectedItemId(id);

    if(triggerItemSavePrompt()) {
        return;
    }

    setSelectedButtonIndex(id, goToSelect);
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
    assetsManager->createPreviewTexture(newestItemIndex);
    selectItem(newestItemIndex, true);
}

void ItemsScrollableWindow::drawItemTypeList(sf::Clock& deltaClock) {
    // --- Left Panel: Scrollable list of textures ---
    ImGui::BeginGroup();

    ImGui::Text("Items list (Max itemType: %d)", (Items::getItemTypesCount() - 1));

    ImVec2 listSize(250, 500);
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
            (ImTextureID) texture->getNativeHandle(),
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

    // Pagination controls
    if (ImGui::Button("<< Page##ItemTypeListPageDec")) {
        decrementPage();
    }
    ImGui::SameLine();

    // Show the current range of IDs
    ImGui::Text("Range: %d-%d", startIndex, endIndex - 1);
    ImGui::SameLine();

    // Input field for navigation with a limited width
    ImGui::SetNextItemWidth(50); // Set a narrower width for the input field
    if (ImGui::InputText("Item Id##ItemTypeIdSearchTextField", idInputBuffer, sizeof(idInputBuffer), ImGuiInputTextFlags_CharsDecimal  | ImGuiInputTextFlags_EnterReturnsTrue)) {
        int inputId = 0;
        try {
            inputId = std::stoi(idInputBuffer); // Convert input text to integer
        } catch (...) {
            Warninger::sendWarning(FUNC_NAME, "Cannot convert input to a number");
        }
        selectItem(inputId);
    }

    ImGui::SameLine();
    if (ImGui::Button("Page >>##ItemTypeListPageInc")) {
        incrementPage();
    }

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

    if(!assetsManager->isDatFileLoaded()) {
        ImGui::EndDisabled();
    }
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
        if (!isValidFolderPath(outputFolder)) {
            ImGui::SameLine();
            ImGui::Text("Invalid path!");
        }
        ImGui::PushItemWidth(200);
        ImGui::InputText("##folder", &outputFolder[0], outputFolder.size() + 1);
        ImGui::SameLine();
        if (ImGui::Button("Browse")) {
            auto selectedFolder = openFileDialogChooseFolder();
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

        auto colorsCount = pushImGuiGray(!isValidFolderPath(outputFolder));
        if (ImGui::Button("Confirm")) {
            if (isValidFolderPath(outputFolder)) {
                exportItem(static_cast<EXPORT_OPTIONS>(exportFormatSelected));
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::PopStyleColor(colorsCount);

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            if(!isValidFolderPath(outputFolder)) {
                outputFolder = getDesktopPath();
            }
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void ItemsScrollableWindow::handleItemTypeImport() {
    auto fileChosen = openFileDialog({"itf", "toml"});

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

    ImGui::BeginChild("PropertiesPanel", ImVec2(450, 500), true);
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

                ImVec2 centeredPos(groupSize.x / 2.0f - ((spriteMaxSize * previewIt->width) / 2.0f), oldPos.y);
                centeredPos.x += spriteMaxSize/4; // Small magic number correction because we couldn't center...
                ImVec2 gridPos = centeredPos; // assignment here is unimportant, we use trick below to get gridPos...
                // There were a lot of problems to draw grid on the preview images, so I settled on a trick.

                for(int h = 0; h < previewIt->height; h++) {
                    for(int w = 0; w < previewIt->width; w++) {
                        int spriteIndex = assetsManager->getTextureIdFromItemType(previewIt, h, w, assetsManager->getAnimationFrameSetting());
                        auto texture = assetsManager->getTexture(spriteIndex);

                        if (texture) {
                            ImVec2 previewSize = ImVec2((float)texture->getSize().x, (float)texture->getSize().y);

                            auto tempPos = centeredPos;
                            tempPos.x += std::floor((float)w * spriteMaxSize);
                            tempPos.y += std::floor(float(h) * spriteMaxSize);
                            ImGui::SetCursorPos(tempPos);

                            ImGui::Image((ImTextureID)texture->getNativeHandle(), previewSize);

                            if (ImGui::IsItemHovered()) {
                                ImGui::SetTooltip("%d", spriteIndex);
                            }

                            // Handle drag-and-drop target
                            if (ImGui::BeginDragDropTarget()) {
                                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("TEXTURE_ID")) {
                                    int newTextureId = *(int *) payload->Data;
                                    assetsManager->setTextureIdFromItemType(previewIt, h, w, assetsManager->getAnimationFrameSetting(), newTextureId); // Update textureId
                                }
                                ImGui::EndDragDropTarget();
                            }

                            // This is a trick to draw grid at a proper position. Very unprofessional, but works.
                            if(w == 0 && h == 0) {
                                gridPos = ImGui::GetItemRectMin();
                            }
                        } else {
                            Warninger::sendWarning(FUNC_NAME, "No texture detected while displaying item's textures");
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
                ImGui::SliderInt("Animation Frame", &assetsManager->getAnimationFrameSettingRef(), 1, previewIt->animationsFrames);
                ImGui::PopItemWidth();

                //ImVec2 gridPos = ImGui::GetItemRectMin(); // ImGui::GetItemRectMin()
                ImVec2 gridTotalSize = ImVec2(spriteMaxSize * previewIt->width, spriteMaxSize * previewIt->height);
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
                    ImGui::Text("Animations:");

                    ImGui::TableNextColumn();
                    ImGui::PushItemWidth(groupSize.x * 0.20);

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
            }
            ImGui::EndTabItem();
        }

        // --- Item Info Tab ---
        if (ImGui::BeginTabItem("Properties")) {
            // Load and display properties checkboxes
            if (ImGui::CollapsingHeader("Flags", ImGuiTreeNodeFlags_DefaultOpen)) {
                // Radio button group for mutually exclusive options
                ImGui::Text("Flag Type:");
                if (ImGui::RadioButton("Common", previewIt->category == COMMON)) {
                    previewIt->category = COMMON;
                };
                if (ImGui::RadioButton("Ground Border", previewIt->category == GROUND_BORDER)) {
                    previewIt->category = GROUND_BORDER;
                };
                if (ImGui::RadioButton("Bottom", previewIt->category == BOTTOM)) {
                    previewIt->category = BOTTOM;
                };
                if (ImGui::RadioButton("Top", previewIt->category == TOP)) {
                    previewIt->category = TOP;
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

    // ------- Save Item Button
    ImGui::SetCursorPosY(propertiesGroupSize.y - 60); // magic number
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + propertiesGroupSize.x - 60); // magic number

    auto colorsCount = pushImGuiGray(!assetsManager->hasUnsavedChanges(CATEGORY_ITEMS_ITEMTYPE));
    if (ImGui::Button("Save Item")) {
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

            assetsManager->createPreviewTexture(getSelectedButtonIndex());
            assetsManager->resetUnsavedItemType();

            selectItem(assetsManager->getLastSelectedItemId(), true);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No")) {
            assetsManager->setUnsavedChanges(CATEGORY_ITEMS_ITEMTYPE, false);

            assetsManager->createPreviewTexture(getSelectedButtonIndex());
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