#include "OutfitsScrollableWindow.h"
#include "Misc/definitions.h"
#include "Misc/Warninger.h"

OutfitsScrollableWindow::OutfitsScrollableWindow(sf::RenderWindow& window, AssetsManager* am)
: window(window), assetsManager(am)
{
    idInputBuffer[0] = '\0';
}

void OutfitsScrollableWindow::selectOutfit(int id, bool goToSelect) {
    if(getSelectedButtonIndex() >= 0 && id == getSelectedButtonIndex()) {
        return;
    }
    if(!Outfits::isValidOutfitTypeIndex(id)) {
        Warninger::sendWarning(FUNC_NAME, "OutfitType that we try to select doesn't exist (" + std::to_string(id) + ")");
        return;
    }

    assetsManager->setAnimationFrameSetting(1);
    // Stop animation when selecting a different outfit
    isAnimationPlaying = false;
    // Reset direction to North when selecting different outfit
    selectedDirection = 0;
    if(goToSelect) {
        scrollToButtonIndex = id;
    }
    selectedOutfitIndex = id;
    
    // Load unsaved copy
    if (id >= 0 && id < (int)Outfits::getOutfitTypesCount()) {
        unsavedOutfitType = std::make_shared<OutfitType>(*Outfits::getOutfitType(id));
        unsavedOutfitTypeId = id;
    }
}

int OutfitsScrollableWindow::addOutfitType() {
    auto newOutfitType = std::make_shared<OutfitType>();
    Outfits::pushOutfitType(newOutfitType);
    return getTotalButtons();
}

bool OutfitsScrollableWindow::removeOutfitType() {
    int selectedIndex = getSelectedButtonIndex();
    if(selectedIndex < 0 || selectedIndex >= getTotalButtons()) {
        return false;
    }
    Outfits::removeOutfitType(selectedIndex);
    if(selectedIndex >= getTotalButtons()) {
        selectedOutfitIndex = getTotalButtons() - 1;
    }
    return true;
}

void OutfitsScrollableWindow::drawOutfitTypeList(sf::Clock& deltaClock) {
    ImGui::BeginGroup();
    ImGui::Text("Outfits list (Max outfitType: %d)", (Outfits::getOutfitTypesCount() > 0 ? (Outfits::getOutfitTypesCount() - 1) : 0));

    ImVec2 listSize(250, 500);
    ImGui::BeginChild("OutfitsList", listSize, true);
    if(!assetsManager->isGraphicFileLoaded() || !assetsManager->isDatFileLoaded()) {
        ImGui::Text("Need to load .dat and .spr!");
        ImGui::EndChild();
        ImGui::EndGroup();
        return;
    }

    // Show message if no outfits loaded
    if (Outfits::getOutfitTypesCount() == 0) {
        ImGui::Text("No outfits loaded. Load a .dat file to see outfits.");
        ImGui::EndChild();
        ImGui::EndGroup();
        return;
    }

    int startIndex = getPageFirstIndex();
    int endIndex = getPageLastIndex();
    
    // Create preview textures for current page if needed
    static int lastOutfitPage = -1;
    if (getCurrentPage() != lastOutfitPage) {
        assetsManager->createPreviewTexturesForPage(startIndex, endIndex - 1, ThingCategory::OUTFIT);
        lastOutfitPage = getCurrentPage();
    }

    for (int i = startIndex; i < endIndex && i < (int)Outfits::getOutfitTypesCount(); ++i) {
        bool isSelected = (i == getSelectedButtonIndex());
        auto texture = assetsManager->getPreviewTexture(i, ThingCategory::OUTFIT);

        ImGui::PushID(i);
        if (ImGui::ImageButton("##OutfitTypeButton", (ImTextureID) texture->getNativeHandle(),
            ConfigManager::getInstance()->getItemButtonSize(), ImVec2(0, 0), ImVec2(1, 1))) {
            selectOutfit(i, false);
        }
        ImGui::PopID();

        if (isSelected) {
            ImVec2 buttonPos = ImGui::GetItemRectMin();
            ImVec2 buttonSize = ImGui::GetItemRectSize();
            ImU32 borderColor = ConfigManager::getInstance()->getImGuiSelectedThingColor();
            ImGui::GetWindowDrawList()->AddRect(buttonPos, ImVec2(buttonPos.x + buttonSize.x, buttonPos.y + buttonSize.y),
                borderColor, 0.0f, ImDrawFlags_None, 3.0f);
        }

        if(isSelected && scrollToButtonIndex == i) {
            ImGui::SetScrollHereY();
            scrollToButtonIndex = -1;
        }

        ImGui::SameLine();
        ImGui::Text("ID: %d", i);
    }

    ImGui::EndChild();
    ImGui::EndGroup();
}

void OutfitsScrollableWindow::drawOutfitTypePanel() {
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

    if (getSelectedButtonIndex() >= 0 && getSelectedButtonIndex() < (int)Outfits::getOutfitTypesCount()) {
        if (!unsavedOutfitType || unsavedOutfitTypeId != getSelectedButtonIndex()) {
            unsavedOutfitType = std::make_shared<OutfitType>(*Outfits::getOutfitType(getSelectedButtonIndex()));
            unsavedOutfitTypeId = getSelectedButtonIndex();
        }

        if (ImGui::BeginTabBar("Tabs")) {
            if (ImGui::BeginTabItem("Texture")) {
                ImGui::BeginGroup();
                ImVec2 groupSize = ImGui::GetContentRegionAvail();
                ImGui::Text("Texture Preview (Drop sprites here)");
                auto oldPos = ImGui::GetCursorPos();
                auto spriteMaxSize = ConfigManager::getInstance()->getSpriteMaxSize();

                ImVec2 centeredPos(groupSize.x / 2.0f - ((spriteMaxSize * unsavedOutfitType->width) / 2.0f), oldPos.y);
                centeredPos.x += spriteMaxSize/4;
                ImVec2 gridPos = centeredPos;

                ImVec2 previewAreaSize(spriteMaxSize * unsavedOutfitType->width, spriteMaxSize * unsavedOutfitType->height);
                ImVec2 previewAreaMin = centeredPos;
                ImVec2 previewAreaMax(previewAreaMin.x + previewAreaSize.x, previewAreaMin.y + previewAreaSize.y);
                ImGui::GetWindowDrawList()->AddRectFilled(previewAreaMin, previewAreaMax, IM_COL32(0, 0, 0, 0));

                for (int l = 0; l < unsavedOutfitType->layers; l++) {
                    for(int w = 0; w < unsavedOutfitType->width; w++) {
                        for(int h = 0; h < unsavedOutfitType->height; h++) {
                            // Use selected direction as patternXIdx (0=North, 1=East, 2=South, 3=West)
                            // Clamp to valid range based on patternX
                            int patternXIdx = selectedDirection;
                            if (patternXIdx >= unsavedOutfitType->patternX) {
                                patternXIdx = 0;
                            }
                            int patternYIdx = 0;
                            int patternZIdx = 0;
                            int spriteIndex = assetsManager->getTextureIdFromThingType(unsavedOutfitType, w, h, assetsManager->getAnimationFrameSetting(), l, patternXIdx, patternYIdx, patternZIdx);
                            auto texture = assetsManager->getTexture(spriteIndex);

                            if (texture) {
                                ImVec2 previewSize = ImVec2((float)texture->getSize().x, (float)texture->getSize().y);
                                auto tempPos = centeredPos;
                                tempPos.x += std::floor((float)(unsavedOutfitType->width - w - 1) * spriteMaxSize);
                                tempPos.y += std::floor((float)(unsavedOutfitType->height - h - 1) * spriteMaxSize);
                                ImGui::SetCursorPos(tempPos);
                                ImGui::Image((ImTextureID)texture->getNativeHandle(), previewSize);
                                
                                // Debug tooltip to show sprite info
                                if (ImGui::IsItemHovered()) {
                                    ImGui::SetTooltip("Sprite: %d\nDirection: %d (patternX=%d)\nPos: w=%d, h=%d, l=%d", 
                                        spriteIndex, selectedDirection, patternXIdx, w, h, l);
                                }

                                if (ImGui::BeginDragDropTarget()) {
                                    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("TEXTURE_ID")) {
                                        int newTextureId = *(int *) payload->Data;
                                        // Use selected direction when dropping sprites
                                        assetsManager->setTextureIdFromThingType(unsavedOutfitType, w, h, assetsManager->getAnimationFrameSetting(), newTextureId, l, selectedDirection, 0, 0);
                                        assetsManager->createPreviewTexture(getSelectedButtonIndex(), ThingCategory::OUTFIT);
                                    }
                                    ImGui::EndDragDropTarget();
                                }

                                if(w == unsavedOutfitType->width - 1 && h == unsavedOutfitType->height - 1 && l == 0) {
                                    gridPos = ImGui::GetItemRectMin();
                                }
                            }
                        }
                    }
                }

                float currentY = ImGui::GetCursorPosY();
                ImGui::SetCursorPosY(currentY + 30);
                ImGui::Checkbox("Draw Grid", &drawGrid);
                ImGui::SameLine();
                float width = ImGui::GetContentRegionAvail().x * 0.15f;
                ImGui::PushItemWidth(width);
                int previousFrame = assetsManager->getAnimationFrameSetting();
                ImGui::SliderInt("Animation Frame", &assetsManager->getAnimationFrameSettingRef(), 1, unsavedOutfitType->animationsFrames);
                // Stop animation if user manually changes the slider
                if (assetsManager->getAnimationFrameSetting() != previousFrame) {
                    isAnimationPlaying = false;
                }
                ImGui::PopItemWidth();

                // Play/Pause Animation Button
                ImGui::SameLine();
                const char* playButtonLabel = isAnimationPlaying ? "||" : ">";
                bool canAnimate = unsavedOutfitType->animationsFrames > 1;
                if (!canAnimate) {
                    isAnimationPlaying = false; // Stop if outfit has only 1 frame
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
                        if (nextFrame > unsavedOutfitType->animationsFrames) {
                            nextFrame = 1;
                        }
                        assetsManager->setAnimationFrameSetting(nextFrame);
                        animationClock.restart();
                    }
                }

                // Direction buttons (only show if patternX > 1)
                // Clamp selected direction to valid range
                if (selectedDirection >= unsavedOutfitType->patternX) {
                    selectedDirection = 0;
                }
                
                if (unsavedOutfitType->patternX > 1) {
                    ImGui::NewLine();
                    ImGui::Text("Direction:");
                    ImGui::SameLine();
                    
                    // North (Up) button - always available if patternX > 1
                    bool northSelected = (selectedDirection == 0);
                    if (northSelected) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                    }
                    if (ImGui::Button("^", ImVec2(30, 30))) {
                        selectedDirection = 0;
                    }
                    if (northSelected) {
                        ImGui::PopStyleColor();
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("North (Up)");
                    }
                    
                    // East (Right) button - only if patternX >= 2
                    if (unsavedOutfitType->patternX >= 2) {
                        ImGui::SameLine();
                        bool eastSelected = (selectedDirection == 1);
                        if (eastSelected) {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                        }
                        // Use ">" for right arrow
                        if (ImGui::Button(">", ImVec2(30, 30))) {
                            selectedDirection = 1;
                        }
                        if (eastSelected) {
                            ImGui::PopStyleColor(2);
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("East (Right)");
                        }
                    }
                    
                    // South (Down) button - only if patternX >= 3
                    if (unsavedOutfitType->patternX >= 3) {
                        ImGui::SameLine();
                        bool southSelected = (selectedDirection == 2);
                        if (southSelected) {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                        }
                        if (ImGui::Button("v", ImVec2(30, 30))) {
                            selectedDirection = 2;
                        }
                        if (southSelected) {
                            ImGui::PopStyleColor();
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("South (Down)");
                        }
                    }
                    
                    // West (Left) button - only if patternX >= 4
                    if (unsavedOutfitType->patternX >= 4) {
                        ImGui::SameLine();
                        bool westSelected = (selectedDirection == 3);
                        if (westSelected) {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                        }
                        if (ImGui::Button("<", ImVec2(30, 30))) {
                            selectedDirection = 3;
                        }
                        if (westSelected) {
                            ImGui::PopStyleColor();
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("West (Left)");
                        }
                    }
                }

                ImVec2 gridTotalSize = ImVec2(spriteMaxSize * unsavedOutfitType->width, spriteMaxSize * unsavedOutfitType->height);
                if (drawGrid) {
                    for (int x = 0; x <= gridTotalSize.x; x += spriteMaxSize) {
                        ImGui::GetWindowDrawList()->AddLine(ImVec2(gridPos.x + x, gridPos.y), ImVec2(gridPos.x + x, gridPos.y + gridTotalSize.y),
                            ConfigManager::getInstance()->getImGuiGridColor(), 1.0f);
                    }
                    for (int y = 0; y <= gridTotalSize.y; y += spriteMaxSize) {
                        ImGui::GetWindowDrawList()->AddLine(ImVec2(gridPos.x, gridPos.y + y), ImVec2(gridPos.x + gridTotalSize.x, gridPos.y + y),
                            ConfigManager::getInstance()->getImGuiGridColor(), 1.0f);
                    }
                }

                ImGui::NewLine();
                ImGui::Separator();
                ImGui::SetCursorPosY(oldPos.y + groupSize.y/2 + 10);

                if (ImGui::BeginTable("OutfitTextureProperties", 3, ImGuiTableFlags_SizingFixedFit)) {
                    ImGui::TableSetupColumn("Column 1", ImGuiTableColumnFlags_WidthFixed, (groupSize.x/2));
                    ImGui::TableSetupColumn("Column 2", ImGuiTableColumnFlags_WidthFixed, (groupSize.x/4));
                    ImGui::TableSetupColumn("Column 3", ImGuiTableColumnFlags_WidthFixed, groupSize.x/4);
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Width:"); ImGui::Text("Height:"); ImGui::Text("Layers:"); ImGui::Text("Pattern X:"); ImGui::Text("Pattern Y:"); ImGui::Text("Pattern Z:"); ImGui::Text("Animations:");

                    ImGui::TableNextColumn();
                    ImGui::PushItemWidth(groupSize.x * 0.20);

                    int width = unsavedOutfitType->width;
                    if (ImGui::InputInt("##Width", &width, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        width = std::clamp(width, 1, ConfigManager::getInstance()->getItemMaxWidth());
                        unsavedOutfitType->setWidth(width);
                    }
                    int height = unsavedOutfitType->height;
                    if (ImGui::InputInt("##Height", &height, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        height = std::clamp(height, 1, ConfigManager::getInstance()->getItemMaxHeight());
                        unsavedOutfitType->setHeight(height);
                    }
                    int layers = unsavedOutfitType->layers;
                    if (ImGui::InputInt("##Layers", &layers, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        layers = std::clamp(layers, 1, 10);
                        unsavedOutfitType->setLayers(layers);
                    }
                    int patternX = unsavedOutfitType->patternX;
                    if (ImGui::InputInt("##PatternX", &patternX, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        patternX = std::clamp(patternX, 1, 10);
                        unsavedOutfitType->setPatternX(patternX);
                    }
                    int patternY = unsavedOutfitType->patternY;
                    if (ImGui::InputInt("##PatternY", &patternY, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        patternY = std::clamp(patternY, 1, 10);
                        unsavedOutfitType->setPatternY(patternY);
                    }
                    int patternZ = unsavedOutfitType->patternZ;
                    if (ImGui::InputInt("##PatternZ", &patternZ, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        patternZ = std::clamp(patternZ, 1, 10);
                        unsavedOutfitType->setPatternZ(patternZ);
                    }
                    int animations = unsavedOutfitType->animationsFrames;
                    if (ImGui::InputInt("##Animations", &animations, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        animations = std::clamp(animations, 1, ConfigManager::getInstance()->getItemMaxAnimationCount());
                        unsavedOutfitType->setAnimationCount(animations);
                        if(animations < assetsManager->getAnimationFrameSetting()) {
                            assetsManager->setAnimationFrameSetting(animations);
                        }
                    }
                    ImGui::PopItemWidth();
                    ImGui::EndTable();
                }
                ImGui::EndGroup();
            } else {
                // Stop animation if no outfit is selected
                isAnimationPlaying = false;
            }
            ImGui::EndTabItem();
        } else {
            // Stop animation when not on texture tab
            isAnimationPlaying = false;
        }
        ImGui::EndTabBar();

        ImGui::SetCursorPosY(propertiesGroupSize.y - 60);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + propertiesGroupSize.x - 60);
        if (ImGui::Button("Save Outfit")) {
            if (unsavedOutfitType && unsavedOutfitTypeId >= 0) {
                Outfits::replaceOutfitType(unsavedOutfitTypeId, std::make_shared<OutfitType>(*unsavedOutfitType));
                assetsManager->createPreviewTexture(unsavedOutfitTypeId, ThingCategory::OUTFIT);
                assetsManager->setUnsavedChanges(CATEGORY_ITEMS, true);
            }
        }
    }

    ImGui::EndChild();
    ImGui::EndGroup();
}

void OutfitsScrollableWindow::drawPaginationControls() {
    if(!assetsManager->isDatFileLoaded()) {
        ImGui::BeginDisabled();
    }

    ImGui::BeginGroup();
    int startIndex = getPageFirstIndex();
    int endIndex = getPageLastIndex();

    if (ImGui::Button("<< Page##OutfitTypeListPageDec")) {
        if(getCurrentPage() > 0) {
            setCurrentPage(getCurrentPage() - 1);
            assetsManager->createPreviewTexturesForPage(getPageFirstIndex(), getPageLastIndex(), ThingCategory::OUTFIT);
        }
    }
    ImGui::SameLine();
    ImGui::Text("Range: %d-%d", startIndex, endIndex - 1);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50);
    if (ImGui::InputText("Outfit Id##OutfitTypeIdSearchTextField", idInputBuffer, sizeof(idInputBuffer), ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
        int inputId = 0;
        try {
            inputId = std::stoi(idInputBuffer);
        } catch (...) {
            Warninger::sendWarning(FUNC_NAME, "Cannot convert input to a number");
        }
        selectOutfit(inputId);
    }
    ImGui::SameLine();
    if (ImGui::Button("Page >>##OutfitTypeListPageInc")) {
        if(getPageLastIndex() < getTotalButtons()) {
            setCurrentPage(getCurrentPage() + 1);
            assetsManager->createPreviewTexturesForPage(getPageFirstIndex(), getPageLastIndex() - 1, ThingCategory::OUTFIT);
        }
    }

    if (ImGui::Button("New Outfit##NewOutfitTypeFromList")) {
        int index = addOutfitType();
        if (index >= 1) {
            selectOutfit(index - 1);
            assetsManager->setUnsavedChanges(CATEGORY_ITEMS, true);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove Outfit##RemoveOutfitTypeFromList")) {
        if(removeOutfitType()) {
            assetsManager->setUnsavedChanges(CATEGORY_ITEMS, true);
        }
    }

    ImGui::EndGroup();
    if(!assetsManager->isDatFileLoaded()) {
        ImGui::EndDisabled();
    }
}

