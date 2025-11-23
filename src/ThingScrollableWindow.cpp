#include "ThingScrollableWindow.h"
#include "Misc/Warninger.h"
#include "ResourceManagers/ConfigManager.h"

ThingScrollableWindow::ThingScrollableWindow(sf::RenderWindow& window, AssetsManager* am, ThingCategory category)
    : window(window), assetsManager(am), category(category) {
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

