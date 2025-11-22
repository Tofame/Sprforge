#include "MissilesScrollableWindow.h"
#include "Misc/definitions.h"
#include "Misc/Warninger.h"

MissilesScrollableWindow::MissilesScrollableWindow(sf::RenderWindow& window, AssetsManager* am)
: window(window), assetsManager(am)
{
    idInputBuffer[0] = '\0';
}

void MissilesScrollableWindow::selectMissile(int id, bool goToSelect) {
    if(getSelectedButtonIndex() >= 0 && id == getSelectedButtonIndex()) return;
    if(!Missiles::isValidMissileTypeIndex(id)) {
        Warninger::sendWarning(FUNC_NAME, "MissileType that we try to select doesn't exist (" + std::to_string(id) + ")");
        return;
    }
    assetsManager->setAnimationFrameSetting(1);
    // Stop animation when selecting a different missile
    isAnimationPlaying = false;
    if(goToSelect) scrollToButtonIndex = id;
    selectedMissileIndex = id;
    if (id >= 0 && id < (int)Missiles::getMissileTypesCount()) {
        unsavedMissileType = std::make_shared<MissileType>(*Missiles::getMissileType(id));
        unsavedMissileTypeId = id;
    }
}

int MissilesScrollableWindow::addMissileType() {
    auto newMissileType = std::make_shared<MissileType>();
    Missiles::pushMissileType(newMissileType);
    return getTotalButtons();
}

bool MissilesScrollableWindow::removeMissileType() {
    int selectedIndex = getSelectedButtonIndex();
    if(selectedIndex < 0 || selectedIndex >= getTotalButtons()) return false;
    Missiles::removeMissileType(selectedIndex);
    if(selectedIndex >= getTotalButtons()) selectedMissileIndex = getTotalButtons() - 1;
    return true;
}

void MissilesScrollableWindow::drawMissileTypeList(sf::Clock& deltaClock) {
    ImGui::BeginGroup();
    ImGui::Text("Missiles list (Max missileType: %d)", (Missiles::getMissileTypesCount() > 0 ? (Missiles::getMissileTypesCount() - 1) : 0));
    ImVec2 listSize(250, 500);
    ImGui::BeginChild("MissilesList", listSize, true);
    if(!assetsManager->isGraphicFileLoaded() || !assetsManager->isDatFileLoaded()) {
        ImGui::Text("Need to load .dat and .spr!");
        ImGui::EndChild();
        ImGui::EndGroup();
        return;
    }
    
    // Show message if no missiles loaded
    if (Missiles::getMissileTypesCount() == 0) {
        ImGui::Text("No missiles loaded. Load a .dat file to see missiles.");
        ImGui::EndChild();
        ImGui::EndGroup();
        return;
    }
    
    int startIndex = getPageFirstIndex();
    int endIndex = getPageLastIndex();
    
    // Create preview textures for current page if needed
    static int lastMissilePage = -1;
    if (getCurrentPage() != lastMissilePage) {
        assetsManager->createPreviewTexturesForPage(startIndex, endIndex - 1, ThingCategory::MISSILE);
        lastMissilePage = getCurrentPage();
    }
    
    for (int i = startIndex; i < endIndex && i < (int)Missiles::getMissileTypesCount(); ++i) {
        bool isSelected = (i == getSelectedButtonIndex());
        auto texture = assetsManager->getPreviewTexture(i, ThingCategory::MISSILE);
        ImGui::PushID(i);
        if (ImGui::ImageButton("##MissileTypeButton", (ImTextureID)(uintptr_t)texture->getNativeHandle(),
            ConfigManager::getInstance()->getItemButtonSize(), ImVec2(0, 0), ImVec2(1, 1))) {
            selectMissile(i, false);
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

void MissilesScrollableWindow::drawMissileTypePanel() {
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
    if (getSelectedButtonIndex() >= 0 && getSelectedButtonIndex() < (int)Missiles::getMissileTypesCount()) {
        if (!unsavedMissileType || unsavedMissileTypeId != getSelectedButtonIndex()) {
            unsavedMissileType = std::make_shared<MissileType>(*Missiles::getMissileType(getSelectedButtonIndex()));
            unsavedMissileTypeId = getSelectedButtonIndex();
        }
        if (ImGui::BeginTabBar("Tabs")) {
            if (ImGui::BeginTabItem("Texture")) {
                ImGui::BeginGroup();
                ImVec2 groupSize = ImGui::GetContentRegionAvail();
                ImGui::Text("Texture Preview (Drop sprites here)");
                auto oldPos = ImGui::GetCursorPos();
                auto spriteMaxSize = ConfigManager::getInstance()->getSpriteMaxSize();
                ImVec2 centeredPos(groupSize.x / 2.0f - ((spriteMaxSize * unsavedMissileType->width) / 2.0f), oldPos.y);
                centeredPos.x += spriteMaxSize/4;
                ImVec2 gridPos = centeredPos;
                
                // For missiles, show all 8 directions in a 3x3 grid (only if patternX >= 3 and patternY >= 3)
                if (unsavedMissileType->patternX >= 3 && unsavedMissileType->patternY >= 3) {
                    // Direction labels and patternX/patternY mapping for each cell
                    struct DirectionCell {
                        const char* label;
                        int patternX;
                        int patternY;
                        bool skip;
                    };
                    DirectionCell cells[3][3] = {
                        {{"NW", 0, 0, false}, {"N", 1, 0, false}, {"NE", 2, 0, false}},
                        {{"W", 0, 1, false}, {"", -1, -1, true}, {"E", 2, 1, false}},
                        {{"SW", 0, 2, false}, {"S", 1, 2, false}, {"SE", 2, 2, false}}
                    };
                    
                    float cellSize = spriteMaxSize * std::max(unsavedMissileType->width, unsavedMissileType->height);
                    float tableWidth = cellSize * 3;
                    
                    // Center the table horizontally
                    ImGui::SetCursorPosX((groupSize.x - tableWidth) / 2.0f);
                    
                    // Use a table to create the grid layout
                    if (ImGui::BeginTable("MissileDirectionsGrid", 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody)) {
                        ImGui::TableSetupColumn("Col1", ImGuiTableColumnFlags_WidthFixed, cellSize);
                        ImGui::TableSetupColumn("Col2", ImGuiTableColumnFlags_WidthFixed, cellSize);
                        ImGui::TableSetupColumn("Col3", ImGuiTableColumnFlags_WidthFixed, cellSize);
                        
                        for (int gridY = 0; gridY < 3; gridY++) {
                            ImGui::TableNextRow();
                            for (int gridX = 0; gridX < 3; gridX++) {
                                ImGui::TableNextColumn();
                                const auto& cell = cells[gridY][gridX];
                                
                                // Skip center cell
                                if (cell.skip) {
                                    ImGui::Text("");
                                    continue;
                                }
                                
                                // Draw direction label
                                ImGui::Text("%s", cell.label);
                                
                                // Reserve space for the preview using an invisible button
                                ImGui::PushID(gridY * 3 + gridX);
                                ImVec2 cellSizeVec(cellSize, cellSize);
                                ImGui::InvisibleButton("##CellButton", cellSizeVec);
                                ImVec2 cellMin = ImGui::GetItemRectMin();
                                ImVec2 cellMax = ImGui::GetItemRectMax();
                                ImGui::GetWindowDrawList()->AddRectFilled(cellMin, cellMax, IM_COL32(30, 30, 30, 255));
                                
                                // Draw preview for this direction using SetCursorPos (like ItemsScrollableWindow)
                                ImVec2 cellStartPos = ImGui::GetCursorPos();
                                for (int l = 0; l < unsavedMissileType->layers; l++) {
                                    for(int w = 0; w < unsavedMissileType->width; w++) {
                                        for(int h = 0; h < unsavedMissileType->height; h++) {
                                            int spriteIndex = assetsManager->getTextureIdFromThingType(unsavedMissileType, w, h, assetsManager->getAnimationFrameSetting(), l, cell.patternX, cell.patternY, 0);
                                            auto texture = assetsManager->getTexture(spriteIndex);
                                            if (texture) {
                                                ImVec2 previewSize = ImVec2((float)texture->getSize().x, (float)texture->getSize().y);
                                                ImVec2 tempPos = cellMin;
                                                tempPos.x += std::floor((float)(unsavedMissileType->width - w - 1) * spriteMaxSize);
                                                tempPos.y += std::floor((float)(unsavedMissileType->height - h - 1) * spriteMaxSize);
                                                ImGui::SetCursorScreenPos(tempPos);
                                                ImGui::Image((ImTextureID)(uintptr_t)texture->getNativeHandle(), previewSize);
                                                
                                                if (ImGui::BeginDragDropTarget()) {
                                                    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("TEXTURE_ID")) {
                                                        int newTextureId = *(int *) payload->Data;
                                                        assetsManager->setTextureIdFromThingType(unsavedMissileType, w, h, assetsManager->getAnimationFrameSetting(), newTextureId, l, cell.patternX, cell.patternY, 0);
                                                        assetsManager->createPreviewTexture(getSelectedButtonIndex(), ThingCategory::MISSILE);
                                                    }
                                                    ImGui::EndDragDropTarget();
                                                }
                                                
                                                if(w == unsavedMissileType->width - 1 && h == unsavedMissileType->height - 1 && l == 0 && gridX == 0 && gridY == 0) {
                                                    gridPos = ImGui::GetItemRectMin();
                                                }
                                            }
                                        }
                                    }
                                }
                                ImGui::PopID();
                            }
                        }
                        ImGui::EndTable();
                    }
                } else {
                    // Fallback: show single direction if patternX/patternY < 3
                    ImVec2 previewAreaSize(spriteMaxSize * unsavedMissileType->width, spriteMaxSize * unsavedMissileType->height);
                    ImVec2 previewAreaMin = centeredPos;
                    ImVec2 previewAreaMax(previewAreaMin.x + previewAreaSize.x, previewAreaMin.y + previewAreaSize.y);
                    ImGui::GetWindowDrawList()->AddRectFilled(previewAreaMin, previewAreaMax, IM_COL32(0, 0, 0, 0));
                    
                    for (int l = 0; l < unsavedMissileType->layers; l++) {
                        for(int w = 0; w < unsavedMissileType->width; w++) {
                            for(int h = 0; h < unsavedMissileType->height; h++) {
                                int spriteIndex = assetsManager->getTextureIdFromThingType(unsavedMissileType, w, h, assetsManager->getAnimationFrameSetting(), l, 0, 0, 0);
                                auto texture = assetsManager->getTexture(spriteIndex);
                                if (texture) {
                                    ImVec2 previewSize = ImVec2((float)texture->getSize().x, (float)texture->getSize().y);
                                    auto tempPos = centeredPos;
                                    tempPos.x += std::floor((float)(unsavedMissileType->width - w - 1) * spriteMaxSize);
                                    tempPos.y += std::floor((float)(unsavedMissileType->height - h - 1) * spriteMaxSize);
                                    ImGui::SetCursorPos(tempPos);
                                    ImGui::Image((ImTextureID)(uintptr_t)texture->getNativeHandle(), previewSize);
                                    if (ImGui::BeginDragDropTarget()) {
                                        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("TEXTURE_ID")) {
                                            int newTextureId = *(int *) payload->Data;
                                            assetsManager->setTextureIdFromThingType(unsavedMissileType, w, h, assetsManager->getAnimationFrameSetting(), newTextureId, l, 0, 0, 0);
                                            assetsManager->createPreviewTexture(getSelectedButtonIndex(), ThingCategory::MISSILE);
                                        }
                                        ImGui::EndDragDropTarget();
                                    }
                                    if(w == unsavedMissileType->width - 1 && h == unsavedMissileType->height - 1 && l == 0) {
                                        gridPos = ImGui::GetItemRectMin();
                                    }
                                }
                            }
                        }
                    }
                }
                
                ImGui::NewLine();
                ImGui::Checkbox("Draw Grid", &drawGrid);
                ImGui::SameLine();
                float width = ImGui::GetContentRegionAvail().x * 0.15f;
                ImGui::PushItemWidth(width);
                int previousFrame = assetsManager->getAnimationFrameSetting();
                ImGui::SliderInt("Animation Frame", &assetsManager->getAnimationFrameSettingRef(), 1, unsavedMissileType->animationsFrames);
                // Stop animation if user manually changes the slider
                if (assetsManager->getAnimationFrameSetting() != previousFrame) {
                    isAnimationPlaying = false;
                }
                ImGui::PopItemWidth();

                // Play/Pause Animation Button
                ImGui::SameLine();
                const char* playButtonLabel = isAnimationPlaying ? "||" : ">";
                bool canAnimate = unsavedMissileType->animationsFrames > 1;
                if (!canAnimate) {
                    isAnimationPlaying = false; // Stop if missile has only 1 frame
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
                        if (nextFrame > unsavedMissileType->animationsFrames) {
                            nextFrame = 1;
                        }
                        assetsManager->setAnimationFrameSetting(nextFrame);
                        animationClock.restart();
                    }
                }


                ImVec2 gridTotalSize = ImVec2(spriteMaxSize * unsavedMissileType->width, spriteMaxSize * unsavedMissileType->height);
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
                ImGui::Separator();
                
                // Set fixed Y-position for properties table (same as ItemsScrollableWindow)
                ImGui::SetCursorPosY(oldPos.y + groupSize.y/2 + 10);
                
                if (ImGui::BeginTable("MissileTextureProperties", 3, ImGuiTableFlags_SizingFixedFit)) {
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
                    ImGui::PushItemWidth(groupSize.x * 0.20);
                    int width = unsavedMissileType->width;
                    if (ImGui::InputInt("##Width", &width, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        width = std::clamp(width, 1, ConfigManager::getInstance()->getItemMaxWidth());
                        unsavedMissileType->setWidth(width);
                    }
                    int height = unsavedMissileType->height;
                    if (ImGui::InputInt("##Height", &height, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        height = std::clamp(height, 1, ConfigManager::getInstance()->getItemMaxHeight());
                        unsavedMissileType->setHeight(height);
                    }
                    int layers = unsavedMissileType->layers;
                    if (ImGui::InputInt("##Layers", &layers, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        layers = std::clamp(layers, 1, 10);
                        unsavedMissileType->setLayers(layers);
                    }
                    int patternX = unsavedMissileType->patternX;
                    if (ImGui::InputInt("##PatternX", &patternX, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        patternX = std::clamp(patternX, 1, 10);
                        unsavedMissileType->setPatternX(patternX);
                    }
                    int patternY = unsavedMissileType->patternY;
                    if (ImGui::InputInt("##PatternY", &patternY, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        patternY = std::clamp(patternY, 1, 10);
                        unsavedMissileType->setPatternY(patternY);
                    }
                    int patternZ = unsavedMissileType->patternZ;
                    if (ImGui::InputInt("##PatternZ", &patternZ, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        patternZ = std::clamp(patternZ, 1, 10);
                        unsavedMissileType->setPatternZ(patternZ);
                    }
                    int animations = unsavedMissileType->animationsFrames;
                    if (ImGui::InputInt("##Animations", &animations, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        animations = std::clamp(animations, 1, ConfigManager::getInstance()->getItemMaxAnimationCount());
                        unsavedMissileType->setAnimationCount(animations);
                        if(animations < assetsManager->getAnimationFrameSetting()) {
                            assetsManager->setAnimationFrameSetting(animations);
                        }
                    }
                    ImGui::PopItemWidth();
                    ImGui::EndTable();
                }
                ImGui::EndGroup();
            } else {
                // Stop animation if no missile is selected
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
        if (ImGui::Button("Save Missile")) {
            if (unsavedMissileType && unsavedMissileTypeId >= 0) {
                Missiles::replaceMissileType(unsavedMissileTypeId, std::make_shared<MissileType>(*unsavedMissileType));
                assetsManager->createPreviewTexture(unsavedMissileTypeId, ThingCategory::MISSILE);
                assetsManager->setUnsavedChanges(CATEGORY_ITEMS, true);
            }
        }
    }
    ImGui::EndChild();
    ImGui::EndGroup();
}

void MissilesScrollableWindow::drawPaginationControls() {
    if(!assetsManager->isDatFileLoaded()) {
        ImGui::BeginDisabled();
    }
    ImGui::BeginGroup();
    int startIndex = getPageFirstIndex();
    int endIndex = getPageLastIndex();
    if (ImGui::Button("<< Page##MissileTypeListPageDec")) {
        if(getCurrentPage() > 0) {
            setCurrentPage(getCurrentPage() - 1);
            assetsManager->createPreviewTexturesForPage(getPageFirstIndex(), getPageLastIndex() - 1, ThingCategory::MISSILE);
        }
    }
    ImGui::SameLine();
    ImGui::Text("Range: %d-%d", startIndex, endIndex - 1);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50);
    if (ImGui::InputText("Missile Id##MissileTypeIdSearchTextField", idInputBuffer, sizeof(idInputBuffer), ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
        int inputId = 0;
        try {
            inputId = std::stoi(idInputBuffer);
        } catch (...) {
            Warninger::sendWarning(FUNC_NAME, "Cannot convert input to a number");
        }
        selectMissile(inputId);
    }
    ImGui::SameLine();
    if (ImGui::Button("Page >>##MissileTypeListPageInc")) {
        if(getPageLastIndex() < getTotalButtons()) {
            setCurrentPage(getCurrentPage() + 1);
            assetsManager->createPreviewTexturesForPage(getPageFirstIndex(), getPageLastIndex() - 1, ThingCategory::MISSILE);
        }
    }
    if (ImGui::Button("New Missile##NewMissileTypeFromList")) {
        int index = addMissileType();
        if (index >= 1) {
            selectMissile(index - 1);
            assetsManager->setUnsavedChanges(CATEGORY_ITEMS, true);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove Missile##RemoveMissileTypeFromList")) {
        if(removeMissileType()) {
            assetsManager->setUnsavedChanges(CATEGORY_ITEMS, true);
        }
    }
    ImGui::EndGroup();
    if(!assetsManager->isDatFileLoaded()) {
        ImGui::EndDisabled();
    }
}

