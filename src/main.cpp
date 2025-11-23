#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include "ItemsScrollableWindow.h"
#include "OutfitsScrollableWindow.h"
#include "EffectsScrollableWindow.h"
#include "MissilesScrollableWindow.h"
#include "SpritesScrollableWindow.h"
#include "Things/Outfits.h"
#include "Things/Effects.h"
#include "Things/Missiles.h"
#include "Misc/tools.h"
#include "Helper/GUIHelper.h"
#include "Helper/SavedData.h"
#include "Misc/definitions.h"
#include "Helper/DropManager.h"

void displayExitConfirmation(sf::RenderWindow& window, bool& showExitConfirmation, bool unsavedChanges, AssetsManager* am);
void pasteFromClipboard(AssetsManager* am, SpritesScrollableWindow* spritesWindow, ItemsScrollableWindow* itemsWindow);
void copyToClipboard(AssetsManager* am, SpritesScrollableWindow* spritesWindow, ItemsScrollableWindow* itemsWindow);

int main() {
#ifdef _WIN32
    // Initialize Windows COM for drag-and-drop
    HRESULT hr = OleInitialize(NULL);
#endif

    // Create a single application window
    sf::RenderWindow window(sf::VideoMode({1100, 800}), "Sprforge");
    window.setFramerateLimit(60);
    // Request focus to prevent focus issues
    window.requestFocus();

    // Setup platform-specific DropManager
#ifdef _WIN32
    DropManagerWindows dropManager;
    void* nativeHandle = window.getNativeHandle();
    dropManager.Initialize(nativeHandle);
#else
    DropManagerSFML dropManager;
    dropManager.Initialize(window.getNativeHandle());
#endif

    // Initialize ImGui-SFML
    if (!ImGui::SFML::Init(window)) {
        return 1; // Failed to initialize ImGui-SFML
    }
    window.resetGLStates();
    
    // Configure ImGui style to reduce "window inside window" appearance
    ImGuiStyle& style = ImGui::GetStyle();
    // Reduce window borders and padding for a more integrated look
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f; // Keep borders for popups/modals
    style.WindowRounding = 0.0f;
    style.FrameRounding = 2.0f;

    auto guiHelper = new GUIHelper();

    // initialize assets manager
    auto assetsManager = new AssetsManager(guiHelper);
    // Create instances of the scrollable windows
    SpritesScrollableWindow spritesScrollableWindow(window, assetsManager);
    ItemsScrollableWindow itemsScrollableWindow(window, assetsManager);
    OutfitsScrollableWindow outfitsScrollableWindow(window, assetsManager);
    EffectsScrollableWindow effectsScrollableWindow(window, assetsManager);
    MissilesScrollableWindow missilesScrollableWindow(window, assetsManager);

    // Add drop manager to panels
    spritesScrollableWindow.setDropManager(&dropManager);

    bool showExitConfirmation = false;
    sf::Clock deltaClock;

    while (window.isOpen()) {
        while (const std::optional<sf::Event> eventOptional = window.pollEvent()) {
            const sf::Event& event = *eventOptional;

            // Process SFML events for ImGui-SFML
            ImGui::SFML::ProcessEvent(window, event);
            
            // Handle focus events to ensure window stays focused
            // Note: Focus events may vary by SFML version
            if (event.is<sf::Event::FocusLost>()) {
                // Window lost focus - could add handling here if needed
            }
            else if (event.is<sf::Event::FocusGained>()) {
                // Window gained focus - ensure it's active
                window.requestFocus();
            }
            // Handle window resize to update viewport for proportional scaling
            else if (const auto* resizeEvent = event.getIf<sf::Event::Resized>()) {
                sf::FloatRect visibleArea(sf::Vector2<float>{0.f,0.f}, sf::Vector2<float>{static_cast<float>(resizeEvent->size.x), static_cast<float>(resizeEvent->size.y)});
                window.setView(sf::View(visibleArea));
            }
    
            if (event.is<sf::Event::Closed>()) {
                if (spritesScrollableWindow.hasUnsavedChanges()) {
                    showExitConfirmation = true;
                } else {
                    window.close();
                }
            }
#ifndef _WIN32
            // Note: SFML 3.0 doesn't support file drop events
            // File drag-and-drop on macOS would need to be implemented using Cocoa drag-and-drop APIs
            // For now, file drops are not supported on macOS/Linux via SFML events
            // TODO: Implement native macOS drag-and-drop using NSView drag-and-drop methods
#endif
            else if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
                // Prevents crash that sometimes happened when ImGui/SFML tried to process weird keys
                if (keyEvent->code <= sf::Keyboard::Key::Unknown) {
                    continue;
                }

                // Hotkey logic: Ctrl+C (Copy); Ctrl+V (Paste)
                // On macOS, Cmd+C and Cmd+V are typically used, but SFML maps them to Control
#ifdef _WIN32
                bool isModifierPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl);
#else
                // On macOS, check for both Cmd and Ctrl
                bool isModifierPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LSystem) ||
                                         sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl);
#endif
                if (isModifierPressed && keyEvent->scancode == sf::Keyboard::Scan::V) {
                    pasteFromClipboard(assetsManager, &spritesScrollableWindow, &itemsScrollableWindow);
                }
                else if (isModifierPressed && keyEvent->scancode == sf::Keyboard::Scan::C) {
                    copyToClipboard(assetsManager, &spritesScrollableWindow, &itemsScrollableWindow);
                }
            }
        }

        // Update ImGui-SFML
        ImGui::SFML::Update(window, deltaClock.restart());

        // Update drop manager
        dropManager.Update(window);

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
        // Make the ImGui window match the SFML window size for proportional scaling
        sf::Vector2u windowSize = window.getSize();
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::Begin("Asset Manager", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);
        
        // Header: Toolbar controls
        assetsManager->drawAssetsManagerControls();
        ImGui::Spacing();
        
        // Category selection tabs
        static int selectedCategory = 0; // 0=Items, 1=Outfits, 2=Effects, 3=Missiles
        if (ImGui::BeginTabBar("ThingCategoryTabs")) {
            if (ImGui::BeginTabItem("Items")) {
                selectedCategory = 0;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Outfits")) {
                selectedCategory = 1;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Effects")) {
                selectedCategory = 2;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Missiles")) {
                selectedCategory = 3;
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Main content area: 30% - 40% - 30% grid layout using columns
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float availableHeight = ImGui::GetContentRegionAvail().y;
        
        // Set up three columns with proper widths
        ImGui::Columns(3, "MainColumns", false);
        ImGui::SetColumnWidth(0, availableWidth * 0.30f);
        ImGui::SetColumnWidth(1, availableWidth * 0.40f);
        ImGui::SetColumnWidth(2, availableWidth * 0.30f);
        
        // Column 1: Thing list (30%)
        ImGui::BeginGroup();
        if (selectedCategory == 0) {
            itemsScrollableWindow.drawItemTypeList(deltaClock);
        } else if (selectedCategory == 1) {
            outfitsScrollableWindow.drawOutfitTypeList(deltaClock);
        } else if (selectedCategory == 2) {
            effectsScrollableWindow.drawEffectTypeList(deltaClock);
        } else if (selectedCategory == 3) {
            missilesScrollableWindow.drawMissileTypeList(deltaClock);
        }
        
        // Controls below list (centered)
        ImGui::Spacing();
        if (assetsManager->isDatFileLoaded() && assetsManager->isGraphicFileLoaded()) {
            float columnWidth = ImGui::GetColumnWidth();
            float columnOffset = ImGui::GetColumnOffset();
            
            // Draw controls in an invisible group to measure width
            ImGui::BeginGroup();
            if (selectedCategory == 0) {
                itemsScrollableWindow.drawPaginationControls();
            } else if (selectedCategory == 1) {
                outfitsScrollableWindow.drawPaginationControls();
            } else if (selectedCategory == 2) {
                effectsScrollableWindow.drawPaginationControls();
            } else if (selectedCategory == 3) {
                missilesScrollableWindow.drawPaginationControls();
            }
            ImGui::EndGroup();
            
            // Center the controls
            float groupWidth = ImGui::GetItemRectSize().x;
            float offset = (columnWidth - groupWidth) * 0.5f;
            if (offset > 0) {
                ImGui::SetCursorPosX(columnOffset + offset);
                // Redraw at centered position
                if (selectedCategory == 0) {
                    itemsScrollableWindow.drawPaginationControls();
                } else if (selectedCategory == 1) {
                    outfitsScrollableWindow.drawPaginationControls();
                } else if (selectedCategory == 2) {
                    effectsScrollableWindow.drawPaginationControls();
                } else if (selectedCategory == 3) {
                    missilesScrollableWindow.drawPaginationControls();
                }
            }
        }
        ImGui::EndGroup();
        
        // Column 2: Thing properties panel (40%)
        ImGui::NextColumn();
        if (selectedCategory == 0) {
            itemsScrollableWindow.drawItemTypePanel();
        } else if (selectedCategory == 1) {
            outfitsScrollableWindow.drawOutfitTypePanel();
        } else if (selectedCategory == 2) {
            effectsScrollableWindow.drawEffectTypePanel();
        } else if (selectedCategory == 3) {
            missilesScrollableWindow.drawMissileTypePanel();
        }
        
        // Column 3: Sprites list (30%)
        ImGui::NextColumn();
        ImGui::BeginGroup();
        spritesScrollableWindow.drawTextureList(deltaClock);
        
        // Controls below sprites list (centered)
        ImGui::Spacing();
        if (assetsManager->isGraphicFileLoaded()) {
            float columnWidth = ImGui::GetColumnWidth();
            float columnOffset = ImGui::GetColumnOffset();
            
            // Draw controls in an invisible group to measure width
            ImGui::BeginGroup();
            spritesScrollableWindow.drawListControlButtons();
            ImGui::EndGroup();
            
            // Center the controls
            float groupWidth = ImGui::GetItemRectSize().x;
            float offset = (columnWidth - groupWidth) * 0.5f;
            if (offset > 0) {
                ImGui::SetCursorPosX(columnOffset + offset);
                // Redraw at centered position
                spritesScrollableWindow.drawListControlButtons();
            }
        }
        ImGui::EndGroup();
        
        ImGui::Columns(1); // Reset columns
        
        // Status message at the bottom (centered)
        if (!assetsManager->isDatFileLoaded() && !assetsManager->isGraphicFileLoaded()) {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            const char* text = "Load assets first before buttons can be active.";
            float textWidth = ImGui::CalcTextSize(text).x;
            float windowWidth = ImGui::GetContentRegionAvail().x;
            float offset = (windowWidth - textWidth) * 0.5f;
            if (offset > 0)
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
            ImGui::Text("%s", text);
        }

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

    // Shutdown drop manager
    dropManager.Shutdown();

#ifdef _WIN32
    OleUninitialize();
#endif

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

#ifdef _WIN32
    bool hasBitmap = IsClipboardFormatAvailable(CF_BITMAP);
    bool hasItemType = IsClipboardFormatAvailable(RegisterClipboardFormat("ItemTypeBinary"));
#else
    // On macOS, we check if clipboard has image data
    bool hasBitmap = Tools::hasImageInClipboard();
    bool hasItemType = Tools::hasItemTypeInClipboard();
#endif

    if(selectedCategory == CATEGORY_SPRITES && hasBitmap) {
        auto pastedTexture = std::make_shared<sf::Texture>();
        if (Tools::pasteTextureFromClipboard(pastedTexture)) {
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
    } else if(selectedCategory == CATEGORY_ITEMS && hasItemType) {
        auto pastedItem = std::make_shared<ItemType>();
        if (Tools::pasteItemTypeFromClipboard(*pastedItem)) {
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
        Tools::copyTextureToClipboard(*am->getTexture(index));
    } else if(selectedCategory == CATEGORY_ITEMS) {
        Tools::copyItemTypeToClipboard(*Items::getItemType(itemsWindow->getSelectedButtonIndex()));
    }
}