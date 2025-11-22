#pragma once

#include <string>
#include <vector>
#include <imgui.h>
#include <SFML/Graphics/RenderWindow.hpp>

#ifdef _WIN32
#include <windows.h>
#include <oleidl.h>
#endif

// Platform-agnostic DropManager class
class DropManager {
public:
    DropManager() = default;
    virtual ~DropManager() = default;

    // Platform-specific initialization
    virtual void Initialize(void* nativeWindowHandle) = 0;
    virtual void Shutdown() = 0;

    // Update method to be called from event loop
    virtual void Update(sf::RenderWindow& window) = 0;

    // Public interface
    bool IsDraggingFiles() const { return isDraggingFiles; }
    const std::vector<std::string>& GetDraggedFiles() const { return draggedFiles; }
    void ClearDraggedFiles() { draggedFiles.clear(); }

protected:
    bool isDraggingFiles = false;
    std::vector<std::string> draggedFiles;
};

#ifdef _WIN32
// Windows-specific implementation using COM drag-and-drop
class DropManagerWindows : public DropManager, public IDropTarget {
public:
    DropManagerWindows() = default;
    ~DropManagerWindows() = default;

    void Initialize(void* nativeWindowHandle) override {
        hwnd = static_cast<HWND>(nativeWindowHandle);
        RegisterDragDrop(hwnd, this);
    }

    void Shutdown() override {
        if (hwnd) {
            RevokeDragDrop(hwnd);
            hwnd = nullptr;
        }
    }

    void Update(sf::RenderWindow& window) override {
        // Windows drag-and-drop is handled via COM callbacks
        // This method can be used for any additional per-frame updates if needed
    }

    // IUnknown methods
    ULONG AddRef() override { return 1; }
    ULONG Release() override { return 0; }

    HRESULT QueryInterface(REFIID riid, void **ppvObject) override {
        if (riid == IID_IDropTarget || riid == IID_IUnknown) {
            *ppvObject = this;
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    // IDropTarget methods
    HRESULT DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override {
        draggedFiles.clear();

        FORMATETC fmt = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM stg;

        if (SUCCEEDED(pDataObj->GetData(&fmt, &stg))) {
            HDROP hDrop = (HDROP)GlobalLock(stg.hGlobal);
            if (hDrop) {
                UINT count = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
                for (UINT i = 0; i < count; ++i) {
                    TCHAR file[MAX_PATH];
                    if (DragQueryFile(hDrop, i, file, MAX_PATH)) {
                        #if defined(UNICODE) || defined(_UNICODE)
                            // TCHAR is wchar_t, convert to UTF-8 std::string
                            int size_needed = WideCharToMultiByte(CP_UTF8, 0, file, -1, NULL, 0, NULL, NULL);
                            std::string strFile(size_needed - 1, 0); // exclude null terminator
                            WideCharToMultiByte(CP_UTF8, 0, file, -1, &strFile[0], size_needed, NULL, NULL);
                            draggedFiles.push_back(strFile);
                        #else
                            // TCHAR is char, no conversion needed, just copy
                            draggedFiles.push_back(std::string(file));
                        #endif
                    }
                }
                GlobalUnlock(stg.hGlobal);
            }
            ReleaseStgMedium(&stg);
        }

        isDraggingFiles = true;

        // Forward MouseDown to ImGui - simulate left mouse down
        ImGuiIO& io = ImGui::GetIO();
        io.MouseDown[0] = true;

        *pdwEffect = DROPEFFECT_COPY;
        return S_OK;
    }

    HRESULT DragLeave() override {
        isDraggingFiles = false;

        // Forward MouseUp to ImGui - simulate left mouse up
        ImGuiIO& io = ImGui::GetIO();
        io.MouseDown[0] = false;

        draggedFiles.clear();
        return S_OK;
    }

    HRESULT DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override {
        // Update ImGui mouse position to current drag point
        POINT screen_pt = { pt.x, pt.y };
        ScreenToClient(hwnd, &screen_pt);

        ImGuiIO& io = ImGui::GetIO();
        io.MousePos = ImVec2((float)screen_pt.x, (float)screen_pt.y);

        *pdwEffect = DROPEFFECT_COPY;
        return S_OK;
    }

    HRESULT Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override {
        // The files are already gathered in DragEnter, you can also re-gather here if you want

        isDraggingFiles = false;

        // Forward MouseUp to ImGui - simulate left mouse up
        ImGuiIO& io = ImGui::GetIO();
        io.MouseDown[0] = false;

        *pdwEffect = DROPEFFECT_COPY;
        return S_OK;
    }

private:
    HWND hwnd = nullptr;
};
#endif

// Placeholder implementation for macOS/Linux
// Note: SFML 3.0 doesn't support file drop events, so file drag-and-drop
// would need to be implemented using native platform APIs (Cocoa on macOS, GTK on Linux)
// For now, file drops are not functional on macOS/Linux
class DropManagerSFML : public DropManager {
public:
    DropManagerSFML() = default;
    ~DropManagerSFML() = default;

    void Initialize(void* nativeWindowHandle) override {
        // TODO: Implement native macOS drag-and-drop using NSView drag-and-drop methods
        // TODO: Implement native Linux drag-and-drop using GTK drag-and-drop
        (void)nativeWindowHandle; // Suppress unused parameter warning
    }

    void Shutdown() override {
        // No cleanup needed currently
    }

    void Update(sf::RenderWindow& window) override {
        // TODO: Implement native drag-and-drop handling here
        (void)window; // Suppress unused parameter warning
    }

    // Call this from native drag-and-drop callbacks when files are dropped
    void HandleFileDrop(const std::vector<std::string>& files) {
        draggedFiles = files;
        // Set isDraggingFiles to false to indicate drop completed (files are ready to process)
        isDraggingFiles = false;
    }

    // Call this when drag operation ends
    void HandleDragEnd() {
        isDraggingFiles = false;
    }
};
