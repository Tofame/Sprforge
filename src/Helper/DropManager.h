#pragma once

#include <windows.h>
#include <oleidl.h>
#include <string>
#include <vector>
#include <imgui.h>

class DropManager : public IDropTarget
{
public:
    ULONG AddRef()  override { return 1; }
    ULONG Release() override { return 0; }

    HRESULT QueryInterface(REFIID riid, void **ppvObject) override
    {
        if (riid == IID_IDropTarget || riid == IID_IUnknown)
        {
            *ppvObject = this;
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    HRESULT DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override
    {
        draggedFiles.clear();

        FORMATETC fmt = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM stg;

        if (SUCCEEDED(pDataObj->GetData(&fmt, &stg)))
        {
            HDROP hDrop = (HDROP)GlobalLock(stg.hGlobal);
            if (hDrop)
            {
                UINT count = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
                for (UINT i = 0; i < count; ++i)
                {
                    TCHAR file[MAX_PATH];
                    if (DragQueryFile(hDrop, i, file, MAX_PATH))
                    {
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

    HRESULT DragLeave() override
    {
        isDraggingFiles = false;

        // Forward MouseUp to ImGui - simulate left mouse up
        ImGuiIO& io = ImGui::GetIO();
        io.MouseDown[0] = false;

        draggedFiles.clear();
        return S_OK;
    }

    HRESULT DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override
    {
        // Update ImGui mouse position to current drag point
        POINT screen_pt = { pt.x, pt.y };
        ScreenToClient(hwnd, &screen_pt);

        ImGuiIO& io = ImGui::GetIO();
        io.MousePos = ImVec2((float)screen_pt.x, (float)screen_pt.y);

        *pdwEffect = DROPEFFECT_COPY;
        return S_OK;
    }

    HRESULT Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override
    {
        // The files are already gathered in DragEnter, you can also re-gather here if you want

        isDraggingFiles = false;

        // Forward MouseUp to ImGui - simulate left mouse up
        ImGuiIO& io = ImGui::GetIO();
        io.MouseDown[0] = false;

        *pdwEffect = DROPEFFECT_COPY;
        return S_OK;
    }

    void SetWindowHandle(HWND wnd) { hwnd = wnd; }

    bool IsDraggingFiles() const { return isDraggingFiles; }
    const std::vector<std::string>& GetDraggedFiles() const { return draggedFiles; }
    void ClearDraggedFiles() { draggedFiles.clear(); }

private:
    HWND hwnd = nullptr;
    bool isDraggingFiles = false;
    std::vector<std::string> draggedFiles;
};