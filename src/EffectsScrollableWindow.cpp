#include "EffectsScrollableWindow.h"
#include "Misc/definitions.h"
#include "Misc/Warninger.h"

EffectsScrollableWindow::EffectsScrollableWindow(sf::RenderWindow& window, AssetsManager* am)
: window(window), assetsManager(am)
{
    idInputBuffer[0] = '\0';
}

void EffectsScrollableWindow::selectEffect(int id, bool goToSelect) {
    if(getSelectedButtonIndex() >= 0 && id == getSelectedButtonIndex()) return;
    if(!Effects::isValidEffectTypeIndex(id)) {
        Warninger::sendWarning(FUNC_NAME, "EffectType that we try to select doesn't exist (" + std::to_string(id) + ")");
        return;
    }
    assetsManager->setAnimationFrameSetting(1);
    // Stop animation when selecting a different effect
    isAnimationPlaying = false;
    if(goToSelect) scrollToButtonIndex = id;
    selectedEffectIndex = id;
    if (id >= 0 && id < (int)Effects::getEffectTypesCount()) {
        unsavedEffectType = std::make_shared<EffectType>(*Effects::getEffectType(id));
        unsavedEffectTypeId = id;
    }
}

int EffectsScrollableWindow::addEffectType() {
    auto newEffectType = std::make_shared<EffectType>();
    Effects::pushEffectType(newEffectType);
    return getTotalButtons();
}

bool EffectsScrollableWindow::removeEffectType() {
    int selectedIndex = getSelectedButtonIndex();
    if(selectedIndex < 0 || selectedIndex >= getTotalButtons()) return false;
    Effects::removeEffectType(selectedIndex);
    if(selectedIndex >= getTotalButtons()) selectedEffectIndex = getTotalButtons() - 1;
    return true;
}

void EffectsScrollableWindow::drawEffectTypeList(sf::Clock& deltaClock) {
    ImGui::BeginGroup();
    ImGui::Text("Effects list (Max effectType: %d)", (Effects::getEffectTypesCount() > 0 ? (Effects::getEffectTypesCount() - 1) : 0));
    ImVec2 listSize(250, 500);
    ImGui::BeginChild("EffectsList", listSize, true);
    if(!assetsManager->isGraphicFileLoaded() || !assetsManager->isDatFileLoaded()) {
        ImGui::Text("Need to load .dat and .spr!");
        ImGui::EndChild();
        ImGui::EndGroup();
        return;
    }
    
    // Show message if no effects loaded
    if (Effects::getEffectTypesCount() == 0) {
        ImGui::Text("No effects loaded. Load a .dat file to see effects.");
        ImGui::EndChild();
        ImGui::EndGroup();
        return;
    }
    
    int startIndex = getPageFirstIndex();
    int endIndex = getPageLastIndex();
    
    // Create preview textures for current page if needed
    static int lastEffectPage = -1;
    if (getCurrentPage() != lastEffectPage) {
        assetsManager->createPreviewTexturesForPage(startIndex, endIndex - 1, ThingCategory::EFFECT);
        lastEffectPage = getCurrentPage();
    }
    
    for (int i = startIndex; i < endIndex && i < (int)Effects::getEffectTypesCount(); ++i) {
        bool isSelected = (i == getSelectedButtonIndex());
        auto texture = assetsManager->getPreviewTexture(i, ThingCategory::EFFECT);
        ImGui::PushID(i);
        if (ImGui::ImageButton("##EffectTypeButton", (ImTextureID)(uintptr_t)texture->getNativeHandle(),
            ConfigManager::getInstance()->getItemButtonSize(), ImVec2(0, 0), ImVec2(1, 1))) {
            selectEffect(i, false);
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

void EffectsScrollableWindow::drawEffectTypePanel() {
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
    if (getSelectedButtonIndex() >= 0 && getSelectedButtonIndex() < (int)Effects::getEffectTypesCount()) {
        if (!unsavedEffectType || unsavedEffectTypeId != getSelectedButtonIndex()) {
            unsavedEffectType = std::make_shared<EffectType>(*Effects::getEffectType(getSelectedButtonIndex()));
            unsavedEffectTypeId = getSelectedButtonIndex();
        }
        if (ImGui::BeginTabBar("Tabs")) {
            if (ImGui::BeginTabItem("Texture")) {
                ImGui::BeginGroup();
                ImVec2 groupSize = ImGui::GetContentRegionAvail();
                ImGui::Text("Texture Preview (Drop sprites here)");
                auto oldPos = ImGui::GetCursorPos();
                auto spriteMaxSize = ConfigManager::getInstance()->getSpriteMaxSize();
                ImVec2 centeredPos(groupSize.x / 2.0f - ((spriteMaxSize * unsavedEffectType->width) / 2.0f), oldPos.y);
                centeredPos.x += spriteMaxSize/4;
                ImVec2 gridPos = centeredPos;
                ImVec2 previewAreaSize(spriteMaxSize * unsavedEffectType->width, spriteMaxSize * unsavedEffectType->height);
                ImVec2 previewAreaMin = centeredPos;
                ImVec2 previewAreaMax(previewAreaMin.x + previewAreaSize.x, previewAreaMin.y + previewAreaSize.y);
                ImGui::GetWindowDrawList()->AddRectFilled(previewAreaMin, previewAreaMax, IM_COL32(0, 0, 0, 0));
                for (int l = 0; l < unsavedEffectType->layers; l++) {
                    for(int w = 0; w < unsavedEffectType->width; w++) {
                        for(int h = 0; h < unsavedEffectType->height; h++) {
                            int spriteIndex = assetsManager->getTextureIdFromThingType(unsavedEffectType, w, h, assetsManager->getAnimationFrameSetting(), l, 0, 0, 0);
                            auto texture = assetsManager->getTexture(spriteIndex);
                            if (texture) {
                                ImVec2 previewSize = ImVec2((float)texture->getSize().x, (float)texture->getSize().y);
                                auto tempPos = centeredPos;
                                tempPos.x += std::floor((float)(unsavedEffectType->width - w - 1) * spriteMaxSize);
                                tempPos.y += std::floor((float)(unsavedEffectType->height - h - 1) * spriteMaxSize);
                                ImGui::SetCursorPos(tempPos);
                                ImGui::Image((ImTextureID)(uintptr_t)texture->getNativeHandle(), previewSize);
                                if (ImGui::BeginDragDropTarget()) {
                                    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("TEXTURE_ID")) {
                                        int newTextureId = *(int *) payload->Data;
                                        assetsManager->setTextureIdFromThingType(unsavedEffectType, w, h, assetsManager->getAnimationFrameSetting(), newTextureId, l, 0, 0, 0);
                                        assetsManager->createPreviewTexture(getSelectedButtonIndex(), ThingCategory::EFFECT);
                                    }
                                    ImGui::EndDragDropTarget();
                                }
                                if(w == unsavedEffectType->width - 1 && h == unsavedEffectType->height - 1 && l == 0) {
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
                ImGui::SliderInt("Animation Frame", &assetsManager->getAnimationFrameSettingRef(), 1, unsavedEffectType->animationsFrames);
                // Stop animation if user manually changes the slider
                if (assetsManager->getAnimationFrameSetting() != previousFrame) {
                    isAnimationPlaying = false;
                }
                ImGui::PopItemWidth();

                // Play/Pause Animation Button
                ImGui::SameLine();
                const char* playButtonLabel = isAnimationPlaying ? "||" : ">";
                bool canAnimate = unsavedEffectType->animationsFrames > 1;
                if (!canAnimate) {
                    isAnimationPlaying = false; // Stop if effect has only 1 frame
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
                        if (nextFrame > unsavedEffectType->animationsFrames) {
                            nextFrame = 1;
                        }
                        assetsManager->setAnimationFrameSetting(nextFrame);
                        animationClock.restart();
                    }
                }
                ImVec2 gridTotalSize = ImVec2(spriteMaxSize * unsavedEffectType->width, spriteMaxSize * unsavedEffectType->height);
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
                if (ImGui::BeginTable("EffectTextureProperties", 3, ImGuiTableFlags_SizingFixedFit)) {
                    ImGui::TableSetupColumn("Column 1", ImGuiTableColumnFlags_WidthFixed, (groupSize.x/2));
                    ImGui::TableSetupColumn("Column 2", ImGuiTableColumnFlags_WidthFixed, (groupSize.x/4));
                    ImGui::TableSetupColumn("Column 3", ImGuiTableColumnFlags_WidthFixed, groupSize.x/4);
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Width:"); ImGui::Text("Height:"); ImGui::Text("Layers:"); ImGui::Text("Pattern X:"); ImGui::Text("Pattern Y:"); ImGui::Text("Pattern Z:"); ImGui::Text("Animations:");
                    ImGui::TableNextColumn();
                    ImGui::PushItemWidth(groupSize.x * 0.20);
                    int width = unsavedEffectType->width;
                    if (ImGui::InputInt("##Width", &width, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        width = std::clamp(width, 1, ConfigManager::getInstance()->getItemMaxWidth());
                        unsavedEffectType->setWidth(width);
                    }
                    int height = unsavedEffectType->height;
                    if (ImGui::InputInt("##Height", &height, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        height = std::clamp(height, 1, ConfigManager::getInstance()->getItemMaxHeight());
                        unsavedEffectType->setHeight(height);
                    }
                    int layers = unsavedEffectType->layers;
                    if (ImGui::InputInt("##Layers", &layers, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        layers = std::clamp(layers, 1, 10);
                        unsavedEffectType->setLayers(layers);
                    }
                    int patternX = unsavedEffectType->patternX;
                    if (ImGui::InputInt("##PatternX", &patternX, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        patternX = std::clamp(patternX, 1, 10);
                        unsavedEffectType->setPatternX(patternX);
                    }
                    int patternY = unsavedEffectType->patternY;
                    if (ImGui::InputInt("##PatternY", &patternY, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        patternY = std::clamp(patternY, 1, 10);
                        unsavedEffectType->setPatternY(patternY);
                    }
                    int patternZ = unsavedEffectType->patternZ;
                    if (ImGui::InputInt("##PatternZ", &patternZ, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        patternZ = std::clamp(patternZ, 1, 10);
                        unsavedEffectType->setPatternZ(patternZ);
                    }
                    int animations = unsavedEffectType->animationsFrames;
                    if (ImGui::InputInt("##Animations", &animations, 1, 1, ImGuiInputTextFlags_CharsDecimal)) {
                        animations = std::clamp(animations, 1, ConfigManager::getInstance()->getItemMaxAnimationCount());
                        unsavedEffectType->setAnimationCount(animations);
                        if(animations < assetsManager->getAnimationFrameSetting()) {
                            assetsManager->setAnimationFrameSetting(animations);
                        }
                    }
                    ImGui::PopItemWidth();
                    ImGui::EndTable();
                }
                ImGui::EndGroup();
            } else {
                // Stop animation if no effect is selected
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
        if (ImGui::Button("Save Effect")) {
            if (unsavedEffectType && unsavedEffectTypeId >= 0) {
                Effects::replaceEffectType(unsavedEffectTypeId, std::make_shared<EffectType>(*unsavedEffectType));
                assetsManager->createPreviewTexture(unsavedEffectTypeId, ThingCategory::EFFECT);
                assetsManager->setUnsavedChanges(CATEGORY_ITEMS, true);
            }
        }
    }
    ImGui::EndChild();
    ImGui::EndGroup();
}

void EffectsScrollableWindow::drawPaginationControls() {
    if(!assetsManager->isDatFileLoaded()) {
        ImGui::BeginDisabled();
    }
    ImGui::BeginGroup();
    int startIndex = getPageFirstIndex();
    int endIndex = getPageLastIndex();
    if (ImGui::Button("<< Page##EffectTypeListPageDec")) {
        if(getCurrentPage() > 0) {
            setCurrentPage(getCurrentPage() - 1);
            assetsManager->createPreviewTexturesForPage(getPageFirstIndex(), getPageLastIndex() - 1, ThingCategory::EFFECT);
        }
    }
    ImGui::SameLine();
    ImGui::Text("Range: %d-%d", startIndex, endIndex - 1);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50);
    if (ImGui::InputText("Effect Id##EffectTypeIdSearchTextField", idInputBuffer, sizeof(idInputBuffer), ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
        int inputId = 0;
        try {
            inputId = std::stoi(idInputBuffer);
        } catch (...) {
            Warninger::sendWarning(FUNC_NAME, "Cannot convert input to a number");
        }
        selectEffect(inputId);
    }
    ImGui::SameLine();
    if (ImGui::Button("Page >>##EffectTypeListPageInc")) {
        if(getPageLastIndex() < getTotalButtons()) {
            setCurrentPage(getCurrentPage() + 1);
            assetsManager->createPreviewTexturesForPage(getPageFirstIndex(), getPageLastIndex() - 1, ThingCategory::EFFECT);
        }
    }
    if (ImGui::Button("New Effect##NewEffectTypeFromList")) {
        int index = addEffectType();
        if (index >= 1) {
            selectEffect(index - 1);
            assetsManager->setUnsavedChanges(CATEGORY_ITEMS, true);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove Effect##RemoveEffectTypeFromList")) {
        if(removeEffectType()) {
            assetsManager->setUnsavedChanges(CATEGORY_ITEMS, true);
        }
    }
    ImGui::EndGroup();
    if(!assetsManager->isDatFileLoaded()) {
        ImGui::EndDisabled();
    }
}

