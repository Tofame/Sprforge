#include "ThingScrollableWindow.h"
#include "Misc/Warninger.h"
#include "ResourceManagers/ConfigManager.h"

ThingScrollableWindow::ThingScrollableWindow(AssetsManager* am, ThingCategory category)
    : assetsManager(am), category(category) {
    idInputBuffer[0] = '\0';
}

void ThingScrollableWindow::incrementPage() {
    if(!onPageChange()) {
        return;
    }

    if(getPageLastIndex() >= getTotalButtons()) {
        return;
    }

    int oldPage = getCurrentPage();
    setCurrentPage(oldPage + 1);
    onPageChanged(oldPage, getCurrentPage());
}

void ThingScrollableWindow::decrementPage() {
    if(!onPageChange()) {
        return;
    }

    if(getCurrentPage() <= 0) {
        return;
    }

    int oldPage = getCurrentPage();
    setCurrentPage(oldPage - 1);
    onPageChanged(oldPage, getCurrentPage());
}

void ThingScrollableWindow::onPageChanged(int oldPage, int newPage, bool autoSelectFirst) {
    if(oldPage == newPage) {
        return;
    }

    // Load preview textures for current page
    assetsManager->createPreviewTexturesForPage(getPageFirstIndex(), getPageLastIndex(), category);

    // Only auto-select first item if requested (e.g., from page navigation buttons)
    if (autoSelectFirst) {
        selectType(getPageFirstIndex());
    }
}

void ThingScrollableWindow::drawPaginationControls() {
    if(!assetsManager->isDatFileLoaded()) {
        ImGui::BeginDisabled();
    }

    ImGui::BeginGroup();

    int startIndex = getPageFirstIndex();
    int endIndex = getPageLastIndex();

    // Get type name for labels
    const char* typeName = ThingCategoryToString(category);
    std::string typeNameCapitalized = typeName;
    if (!typeNameCapitalized.empty()) {
        typeNameCapitalized[0] = std::toupper(typeNameCapitalized[0]);
    }

    // Pagination controls - first row
    drawPaginationRow(startIndex, endIndex, typeNameCapitalized.c_str(), (std::string(typeNameCapitalized) + " Id").c_str());

    ImGui::Spacing();

    // Action buttons - second row
    drawActionButtonsRow(
        (std::string("New ") + typeNameCapitalized).c_str(),
        (std::string("Remove ") + typeNameCapitalized).c_str()
    );

    ImGui::EndGroup();

    if(!assetsManager->isDatFileLoaded()) {
        ImGui::EndDisabled();
    }
}

void ThingScrollableWindow::drawPaginationRow(int startIndex, int endIndex, const char* typeName, const char* inputLabel) {
    ImGui::BeginGroup();
    
    if (ImGui::Button(("<< Page##" + getPageDecButtonId()).c_str())) {
        decrementPage();
    }
    ImGui::SameLine();
    ImGui::Text("Range: %d-%d", startIndex, endIndex - 1);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50);
    
    std::string inputId = std::string(inputLabel) + "##" + getInputTextFieldId();
    if (ImGui::InputText(inputId.c_str(), idInputBuffer, sizeof(idInputBuffer), ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
        int inputIdValue = 0;
        try {
            inputIdValue = std::stoi(idInputBuffer);
        } catch (...) {
            Warninger::sendWarning(FUNC_NAME, "Cannot convert input to a number");
        }
        selectType(inputIdValue);
    }
    
    ImGui::SameLine();
    if (ImGui::Button(("Page >>##" + getPageIncButtonId()).c_str())) {
        incrementPage();
    }
    
    ImGui::EndGroup();
}

void ThingScrollableWindow::drawActionButtonsRow(const char* newButtonLabel, const char* removeButtonLabel) {
    ImGui::BeginGroup();
    
    if (ImGui::Button((newButtonLabel + std::string("##") + getNewButtonId()).c_str())) {
        int index = addType();
        if (index >= 1) {
            selectType(index - 1);
            assetsManager->setUnsavedChanges(CATEGORY_ITEMS, true);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button((removeButtonLabel + std::string("##") + getRemoveButtonId()).c_str())) {
        if(removeType()) {
            assetsManager->setUnsavedChanges(CATEGORY_ITEMS, true);
        }
    }
    
    ImGui::EndGroup();
}

std::string ThingScrollableWindow::getPageDecButtonId() const {
    return std::string(ThingCategoryToString(category)) + "TypeListPageDec";
}

std::string ThingScrollableWindow::getPageIncButtonId() const {
    return std::string(ThingCategoryToString(category)) + "TypeListPageInc";
}

std::string ThingScrollableWindow::getInputTextFieldId() const {
    return std::string(ThingCategoryToString(category)) + "TypeIdSearchTextField##" + std::string(ThingCategoryToString(category)) + "Id";
}

std::string ThingScrollableWindow::getNewButtonId() const {
    return std::string("New") + ThingCategoryToString(category) + "TypeFromList";
}

std::string ThingScrollableWindow::getRemoveButtonId() const {
    return std::string("Remove") + ThingCategoryToString(category) + "TypeFromList";
}

void ThingScrollableWindow::drawLightControlSegment(ThingType& thing) {
    uint16_t& lightColor = thing.lightBlock.lightColor;
    uint16_t& lightIntensity = thing.lightBlock.lightIntensity;

    bool open = ImGui::CollapsingHeader("Light Settings##LightControlSegment", ImGuiTreeNodeFlags_DefaultOpen);
    const float labelWidth = 120.0f;
    if (open) {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Light Color:");
        ImGui::SameLine(labelWidth);

        int colorValue = thing.lightBlock.lightColor;
        ImGui::SetNextItemWidth(labelWidth);
        if (ImGui::InputInt("##LightControlColorInt", &colorValue, 1, 5)) {
            colorValue = std::clamp(colorValue, 0, 215);
            lightColor = colorValue;
        }

        // Compute current RGB
        int cr = ((lightColor / 36) % 6) * 51;
        int cg = ((lightColor / 6) % 6) * 51;
        int cb = (lightColor % 6) * 51;

        ImVec4 currentColor = ImVec4(cr / 255.0f, cg / 255.0f, cb / 255.0f, 1.0f);

        ImGui::SameLine();

        // The trigger button
        if (ImGui::ColorButton("##CurrentLightControlColor", currentColor,
            ImGuiColorEditFlags_NoTooltip, ImVec2(30, 30)))
        {
            ImGui::OpenPopup("LightControlColorPicker");
        }

        // Popup with color grid
        if (ImGui::BeginPopup("LightControlColorPicker"))
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
                    lightColor = idx;
                    ImGui::CloseCurrentPopup();
                }

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("ID: %d\nR: %d\nG: %d\nB: %d", idx, r, g, b);
                }

                // highlight selected color
                if (lightColor == idx)
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

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Light Intensity:");
        ImGui::SameLine(labelWidth);

        int intensityValue = thing.lightBlock.lightIntensity;
        ImGui::SetNextItemWidth(labelWidth);
        if (ImGui::InputInt("##LightControlIntensityInt", &intensityValue, 1, 5)) {
            intensityValue = std::clamp(intensityValue, 0, 10);
            lightIntensity = intensityValue;
        }

        ImGui::NewLine();
    }
}