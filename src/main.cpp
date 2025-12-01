#include <algorithm>
#include <cstdio>

#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include <GLFW/glfw3.h>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windows.h>
#include <ole2.h>
#endif

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Graphics/SFMLCompat.h"
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
#include <glad/glad.h>

struct AppContext {
    GLFWwindow* window = nullptr;
    AssetsManager* assetsManager = nullptr;
    SpritesScrollableWindow* spritesWindow = nullptr;
    ItemsScrollableWindow* itemsWindow = nullptr;
    bool showExitConfirmation = false;
};

void displayExitConfirmation(GLFWwindow* window, bool& showExitConfirmation, bool unsavedChanges, AssetsManager* am);
void pasteFromClipboard(AssetsManager* am, SpritesScrollableWindow* spritesWindow, ItemsScrollableWindow* itemsWindow);
void copyToClipboard(AssetsManager* am, SpritesScrollableWindow* spritesWindow, ItemsScrollableWindow* itemsWindow);

static void glfwErrorCallback(int error, const char* description) {
    std::fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static void FramebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

static void WindowCloseCallback(GLFWwindow* window) {
    auto ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (!ctx) {
        return;
    }
    if (ctx->spritesWindow && ctx->spritesWindow->hasUnsavedChanges()) {
        ctx->showExitConfirmation = true;
        glfwSetWindowShouldClose(window, GLFW_FALSE);
    }
}

static void CharCallback(GLFWwindow* window, unsigned int codepoint) {
    ImGui_ImplGlfw_CharCallback(window, codepoint);
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
}

static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
}

static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
}

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Forward to ImGui first
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    
    // Only process our custom shortcuts if ImGui doesn't want to capture keyboard input
    // This allows text input fields to work properly (Enter, Backspace, etc.)
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) {
        return;
    }

    if (action != GLFW_PRESS) {
        return;
    }

    auto ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (!ctx || !ctx->assetsManager || !ctx->spritesWindow || !ctx->itemsWindow) {
        return;
    }

#if defined(__APPLE__)
    bool modifierPressed = (mods & GLFW_MOD_SUPER) != 0 || (mods & GLFW_MOD_CONTROL) != 0;
#else
    bool modifierPressed = (mods & GLFW_MOD_CONTROL) != 0;
#endif
    if (!modifierPressed) {
        return;
    }

    if (key == GLFW_KEY_V) {
        pasteFromClipboard(ctx->assetsManager, ctx->spritesWindow, ctx->itemsWindow);
    } else if (key == GLFW_KEY_C) {
        copyToClipboard(ctx->assetsManager, ctx->spritesWindow, ctx->itemsWindow);
    }
}

static void SetupImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;
    style.WindowRounding = 0.0f;
    style.FrameRounding = 2.0f;
}

int main() {
    bool comInitialized = false;
#ifdef _WIN32
    comInitialized = SUCCEEDED(OleInitialize(nullptr));
#endif

    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
#ifdef _WIN32
        if (comInitialized) {
            OleUninitialize();
        }
#endif
        return 1;
    }

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1100, 800, "Sprforge", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
#ifdef _WIN32
        if (comInitialized) {
            OleUninitialize();
        }
#endif
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::fprintf(stderr, "Failed to load OpenGL context via GLAD.\n");
        glfwDestroyWindow(window);
        glfwTerminate();
#ifdef _WIN32
        if (comInitialized) {
            OleUninitialize();
        }
#endif
        return 1;
    }

    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

#ifdef _WIN32
    DropManagerWindows dropManager;
    dropManager.Initialize(glfwGetWin32Window(window));
#else
    DropManagerStub dropManager;
    dropManager.Initialize(nullptr);
#endif

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // Set install_callbacks to false since we'll handle callbacks manually
    // This allows us to forward events to ImGui while also handling custom shortcuts
    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplOpenGL3_Init("#version 330");
    SetupImGuiStyle();

    auto guiHelper = new GUIHelper();
    auto assetsManager = new AssetsManager(guiHelper);
    SpritesScrollableWindow spritesScrollableWindow(assetsManager);
    ItemsScrollableWindow itemsScrollableWindow(assetsManager);
    OutfitsScrollableWindow outfitsScrollableWindow(assetsManager);
    EffectsScrollableWindow effectsScrollableWindow(assetsManager);
    MissilesScrollableWindow missilesScrollableWindow(assetsManager);

    spritesScrollableWindow.setDropManager(&dropManager);

    AppContext appContext;
    appContext.window = window;
    appContext.assetsManager = assetsManager;
    appContext.spritesWindow = &spritesScrollableWindow;
    appContext.itemsWindow = &itemsScrollableWindow;

    glfwSetWindowUserPointer(window, &appContext);
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCharCallback(window, CharCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwSetWindowCloseCallback(window, WindowCloseCallback);

    sf::Clock deltaClock;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        dropManager.Update();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (dropManager.IsDraggingFiles()) {
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceExtern)) {
                ImGui::SetDragDropPayload("FILES", nullptr, 0);
                ImGui::BeginTooltip();
                ImGui::Text("Dragging files...");
                ImGui::EndTooltip();
                ImGui::EndDragDropSource();
            }
        }

        int frameBufferWidth = 0;
        int frameBufferHeight = 0;
        glfwGetFramebufferSize(window, &frameBufferWidth, &frameBufferHeight);
        sf::Vector2u windowSize{
            static_cast<unsigned>(std::max(frameBufferWidth, 0)),
            static_cast<unsigned>(std::max(frameBufferHeight, 0))
        };

        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::Begin("Asset Manager", nullptr,
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoCollapse);

        assetsManager->drawAssetsManagerControls();
        ImGui::Spacing();

        static int selectedCategory = 0;
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

        float availableWidth = ImGui::GetContentRegionAvail().x;
        ImGui::Columns(3, "MainColumns", false);
        ImGui::SetColumnWidth(0, availableWidth * 0.30f);
        ImGui::SetColumnWidth(1, availableWidth * 0.40f);
        ImGui::SetColumnWidth(2, availableWidth * 0.30f);

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

        ImGui::Spacing();
        if (assetsManager->isDatFileLoaded()) {
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
        ImGui::EndGroup();

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

        ImGui::NextColumn();
        ImGui::BeginGroup();
        spritesScrollableWindow.drawTextureList(deltaClock);
        ImGui::Spacing();
        if (assetsManager->isGraphicFileLoaded()) {
            spritesScrollableWindow.drawListControlButtons();
        }
        ImGui::EndGroup();

        ImGui::Columns(1);

        if (!assetsManager->isDatFileLoaded() && !assetsManager->isGraphicFileLoaded()) {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            const char* text = "Load assets first before buttons can be active.";
            float textWidth = ImGui::CalcTextSize(text).x;
            float innerWidth = ImGui::GetContentRegionAvail().x;
            float offset = (innerWidth - textWidth) * 0.5f;
            if (offset > 0.0f) {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
            }
            ImGui::Text("%s", text);
        }

        ImGui::End();

        glClearColor(0.08f, 0.08f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        displayExitConfirmation(window,
                                appContext.showExitConfirmation,
                                assetsManager->isCompilable(),
                                assetsManager);

        ImGui::Render();
        glfwGetFramebufferSize(window, &frameBufferWidth, &frameBufferHeight);
        glViewport(0, 0, frameBufferWidth, frameBufferHeight);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    SavedData::getInstance()->saveData();

    delete assetsManager;
    delete guiHelper;

    dropManager.Shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
#ifdef _WIN32
    if (comInitialized) {
        OleUninitialize();
    }
#endif

    return 0;
}

void displayExitConfirmation(GLFWwindow* window, bool& showExitConfirmation, bool unsavedChanges, AssetsManager* am) {
    if (showExitConfirmation) {
        ImGui::OpenPopup("Exit Confirmation");
        showExitConfirmation = false;
    }

    if (ImGui::BeginPopupModal("Exit Confirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (unsavedChanges) {
            ImGui::Text("You have unsaved changes. Do you really want to exit?");
        } else {
            ImGui::Text("Do you really want to exit?");
        }
        ImGui::Separator();

        if (ImGui::Button("Exit")) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Compile Assets & Exit")) {
            if (am && am->isCompilable()) {
                am->compile();
            }
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine(350);
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void pasteFromClipboard(AssetsManager* am, SpritesScrollableWindow* spritesWindow, ItemsScrollableWindow* itemsWindow) {
    auto selectedCategory = am->getLastSelectedCategory();

#ifdef _WIN32
    bool hasBitmap = IsClipboardFormatAvailable(CF_BITMAP);
    bool hasItemType = IsClipboardFormatAvailable(RegisterClipboardFormatA("ItemTypeBinary"));
#else
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
