#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include "ItemsScrollableWindow.h"
#include "SpritesScrollableWindow.h"
#include "Misc/tools.h"
#include "Helper/GUIHelper.h"
#include "Helper/SavedData.h"
#include "Misc/definitions.h"
#include "Helper/DropManager.h"

void displayExitConfirmation(sf::RenderWindow& window, bool& showExitConfirmation, bool unsavedChanges, AssetsManager* am);
void pasteFromClipboard(AssetsManager* am, SpritesScrollableWindow* spritesWindow, ItemsScrollableWindow* itemsWindow);
void copyToClipboard(AssetsManager* am, SpritesScrollableWindow* spritesWindow, ItemsScrollableWindow* itemsWindow);

int main() {
    HRESULT hr = OleInitialize(NULL);

    // Create a single application window
    sf::RenderWindow window(sf::VideoMode({1100, 800}), "Sprforge");
    window.setFramerateLimit(60);

    // Setup and register your DropManager here
    DropManager dropManager;
    HWND hwnd = static_cast<HWND>(window.getNativeHandle());
    dropManager.SetWindowHandle(hwnd);
    RegisterDragDrop(hwnd, &dropManager);

    // Initialize ImGui-SFML
    ImGui::SFML::Init(window);
    window.resetGLStates();

    auto guiHelper = new GUIHelper();

    // initialize assets manager
    auto assetsManager = new AssetsManager(guiHelper);
    // Create an instance of the SpritesScrollableWindow
    SpritesScrollableWindow spritesScrollableWindow(window, assetsManager);
    ItemsScrollableWindow itemsScrollableWindow(window, assetsManager);

    // Add drop manager to panels
    spritesScrollableWindow.setDropManager(&dropManager);

    bool showExitConfirmation = false;
    sf::Clock deltaClock;

    while (window.isOpen()) {
        while (const std::optional<sf::Event> eventOptional = window.pollEvent()) {
            const sf::Event& event = *eventOptional;

            // Process SFML events for ImGui-SFML
            ImGui::SFML::ProcessEvent(window, event);

            if (event.is<sf::Event::Closed>()) {
                if (spritesScrollableWindow.hasUnsavedChanges()) {
                    showExitConfirmation = true;
                } else {
                    window.close();
                }
            }
            else if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
                // Prevents crash that sometimes happened when ImGui/SFML tried to process weird keys
                if (keyEvent->code <= sf::Keyboard::Key::Unknown) {
                    continue;
                }

                // Hotkey logic: Ctrl+C (Copy); Ctrl+V (Paste)
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) && keyEvent->scancode == sf::Keyboard::Scan::V) {
                    pasteFromClipboard(assetsManager, &spritesScrollableWindow, &itemsScrollableWindow);
                }
                else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) && keyEvent->scancode == sf::Keyboard::Scan::C) {
                    copyToClipboard(assetsManager, &spritesScrollableWindow, &itemsScrollableWindow);
                }
            }
        }

        // Update ImGui-SFML
        ImGui::SFML::Update(window, deltaClock.restart());

        // For showing demo window
        //ImGui::ShowDemoWindow();

        if (dropManager.IsDraggingFiles())
        {
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceExtern))
            {
                // Tell ImGui we have an external drag source — files dragged from Windows Explorer
                ImGui::SetDragDropPayload("FILES", nullptr, 0);  // no actual payload data needed here
                ImGui::BeginTooltip();
                ImGui::Text("Dragging files...");
                ImGui::EndTooltip();
                ImGui::EndDragDropSource();
            }
        }

        // Call the update method of the SpritesScrollableWindow
        ImGui::Begin("Asset Manager");
        assetsManager->drawAssetsManagerControls();
        itemsScrollableWindow.drawItemTypeList(deltaClock);
        ImGui::SameLine();
        itemsScrollableWindow.drawItemTypePanel();
        ImGui::SameLine();
        spritesScrollableWindow.drawTextureList(deltaClock);

        ImGui::Separator();
        if (!assetsManager->isDatFileLoaded() && !assetsManager->isGraphicFileLoaded()) {
            const char* text = "Load assets first before buttons can be active.";
            float windowWidth = ImGui::GetContentRegionAvail().x;
            float textWidth = ImGui::CalcTextSize(text).x;

            float offset = (windowWidth - textWidth) * 0.5f;
            if (offset > 0)
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

            ImGui::Text("%s", text);
        }
        itemsScrollableWindow.drawPaginationControls();
        ImGui::SameLine(600);
        spritesScrollableWindow.drawListControlButtons();

        ImGui::End();

        // Clear the SFML window
        window.clear();

        displayExitConfirmation(window, showExitConfirmation, assetsManager->isCompilable(), assetsManager);
        // Render ImGui on top of the existing SFML content
        ImGui::SFML::Render(window);
        window.display(); // Display everything in the SFML window
    }

    // Cleanup
    SavedData::getInstance()->saveData();

    delete assetsManager;
    delete guiHelper;

    ImGui::SFML::Shutdown();

    RevokeDragDrop(hwnd);
    OleUninitialize();

    return 0;
}

void displayExitConfirmation(sf::RenderWindow& window, bool& showExitConfirmation, bool unsavedChanges, AssetsManager* am) {
    // Check if we should show the exit confirmation dialog
    if (showExitConfirmation) {
        ImGui::OpenPopup("Exit Confirmation");
        showExitConfirmation = false; // Reset the flag to avoid reopening the popup continuously
    }

    // Create the exit confirmation modal
    if (ImGui::BeginPopupModal("Exit Confirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("You have unsaved changes. Do you really want to exit?");
        ImGui::Separator();

        // Handle exit confirmation
        if (ImGui::Button("Exit")) {
            window.close(); // Close the window if the user confirms
            ImGui::CloseCurrentPopup(); // Close the popup
        }
        ImGui::SameLine();
        if (ImGui::Button("Compile Assets & Exit")) {
            // TO-DO make better check, instead of compilable. E.g. sometimes we need to prompt "Compile As" basically.
            if(am->isCompilable()) {
                am->compile();
            }

            window.close(); // Close the window if the user confirms
            ImGui::CloseCurrentPopup(); // Close the popup
        }
        ImGui::SameLine(350);
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup(); // Close the popup if cancelled
        }
        ImGui::EndPopup();
    }
}

void pasteFromClipboard(AssetsManager* am, SpritesScrollableWindow* spritesWindow, ItemsScrollableWindow* itemsWindow) {
    auto selectedCategory = am->getLastSelectedCategory();

    if(selectedCategory == CATEGORY_SPRITES && IsClipboardFormatAvailable(CF_BITMAP)) {
        auto pastedTexture = std::make_shared<sf::Texture>();
        if (pasteTextureFromClipboard(pastedTexture)) {
            auto spriteMaxSize = ConfigManager::getInstance()->getSpriteMaxSize();
            if (pastedTexture->getSize().x != spriteMaxSize ||
                pastedTexture->getSize().y != spriteMaxSize)
            {
                Warninger::sendWarning(FUNC_NAME,
                                       "Clipboard Texture: texture size must be "
                                       + std::to_string(spriteMaxSize) + "x" + std::to_string(spriteMaxSize));
                return;
            }

            int index = spritesWindow->getSelectedSpriteIndex();
            am->replaceTexture(index, pastedTexture);
        } else {
            Warninger::sendWarning(FUNC_NAME, "Unable to paste Texture from clipboard");
        }
    } else if(selectedCategory == CATEGORY_ITEMS && IsClipboardFormatAvailable(RegisterClipboardFormat("ItemTypeBinary"))) {
        auto pastedItem = std::make_shared<ItemType>();
        if (pasteItemTypeFromClipboard(*pastedItem)) {
            // Logic to replace the item with the pasted one
            int index = itemsWindow->getSelectedButtonIndex();
            am->setUnsavedItemType(pastedItem, index);
        } else {
            Warninger::sendWarning(FUNC_NAME, "Unable to paste ItemType from clipboard");
        }
    }
}

void copyToClipboard(AssetsManager* am, SpritesScrollableWindow* spritesWindow, ItemsScrollableWindow* itemsWindow) {
    auto selectedCategory = am->getLastSelectedCategory();

    if(selectedCategory == CATEGORY_SPRITES) {
        int index = spritesWindow->getSelectedSpriteIndex();
        copyTextureToClipboard(*am->getTexture(index));
    } else if(selectedCategory == CATEGORY_ITEMS) {
        copyItemTypeToClipboard(*Items::getItemType(itemsWindow->getSelectedButtonIndex()));
    }
}