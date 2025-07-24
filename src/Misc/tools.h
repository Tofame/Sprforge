#pragma once

#include <string>
#include "imgui.h"
#include "nfd.h"
#include <algorithm>
#include <SFML/Graphics.hpp>
#include "../Things/ItemType.h"
#include "../Things/Items.h"

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#elif __APPLE__
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#elif __linux__
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

inline static void ensureAlphaChannel(sf::Image& image);
inline static void imageRemoveMagenta(sf::Image& image);

enum EXPORT_OPTIONS {
    PNG = 0,
    BMP = 1,
    JPG = 2,
    ITF = 3,
    TOML = 4
};

inline std::string getFormatString(EXPORT_OPTIONS option) {
    switch(option) {
        case PNG:
            return ".png";
        case BMP:
            return ".bmp";
        case JPG:
            return ".jpg";
        case TOML:
            return ".toml";
        case ITF:
            return ".itf";
        default:
            return ".png";
    }
}

inline std::string openFileDialog(const std::vector<std::string>& extensions) {
    NFD_Init();
    nfdu8char_t *outPath = nullptr;

    // Create filter items dynamically based on input extensions
    int filtersCount = (int)extensions.size();
    nfdu8filteritem_t filters[filtersCount];
    for(int i = 0; i < filtersCount; i++) {
        const auto& ext = extensions[i];
        filters[i] = { (ext + " Files").c_str(), ext.c_str() };
    }

    nfdopendialogu8args_t args = {0};
    args.filterList = filters;
    args.filterCount = filtersCount;

    // Open the file dialog
    nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
    std::string chosenPath;

    if (result == NFD_OKAY) {
        chosenPath = outPath;
        NFD_FreePathU8(outPath);
    }
    else if (result == NFD_CANCEL) {
        puts("File dialogue cancelled.");
    }
    else {
        printf("Error: %s\n", NFD_GetError());
    }

    NFD_Quit();
    return chosenPath;
}

inline std::string getDesktopPath() {
    std::string desktopPath;

#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOP, NULL, 0, path))) {
        desktopPath = path;
    } else {
        desktopPath = "C:\\Users\\Default\\Desktop";  // Fallback
    }
#elif __APPLE__ || __linux__
    struct passwd *pw = getpwuid(getuid());
    if (pw && pw->pw_dir) {
        desktopPath = std::string(pw->pw_dir) + "/Desktop";
    } else {
        desktopPath = "/home/default/Desktop";  // Fallback
    }
#endif

    return desktopPath;
}

inline std::string openFileDialogChooseFolder() {
    NFD_Init();
    nfdu8char_t* outPath = nullptr;

    nfdresult_t result = NFD_PickFolder(&outPath, nullptr);
    std::string chosenFolderPath;

    if (result == NFD_OKAY) {
        chosenFolderPath = outPath;
        NFD_FreePathU8(outPath);
    }
    else if (result == NFD_CANCEL) {
        puts("User pressed cancel.");
    }
    else {
        printf("Error: %s\n", NFD_GetError());
    }

    NFD_Quit();
    return chosenFolderPath;
}

inline bool isValidFolderPath(const std::string& path) {
    return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}

inline bool isPresentFileExtensionInAPath(const std::string& path, std::string extension) {
    if (!std::filesystem::is_directory(path)) {
        return false;
    }

    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (std::filesystem::is_regular_file(entry)) {
            std::string fileExtension = entry.path().extension().string();
            if (fileExtension == extension) {
                return true;
            }
        }
    }

    return false;
}

inline std::string findFile(const std::string& path, const std::string& extension) {
    if (!std::filesystem::is_directory(path)) {
        return "";
    }

    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (std::filesystem::is_regular_file(entry)) {
            if (entry.path().extension() == extension) {
                return entry.path().string();
            }
        }
    }

    return "";
}

inline void removeSuffix(std::string& str, const std::string& suffix) {
    if (str.size() >= suffix.size() && str.rfind(suffix) == str.size() - suffix.size()) {
        str.erase(str.size() - suffix.size());
    }
}

inline std::string getSuffix(const std::string& filePath) {
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos == std::string::npos) {
        return ""; // No extension found
    }
    return filePath.substr(dotPos); // Includes the dot
}

inline std::string formatDuration(std::chrono::milliseconds duration) {
    int minutes = duration.count() / 60000;
    int seconds = (duration.count() % 60000) / 1000;
    return std::to_string(minutes) + " min " + std::to_string(seconds) + " sec";
}

inline std::string trim(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(), ::isspace);
    auto end = std::find_if_not(str.rbegin(), str.rend(), ::isspace).base();
    return (start < end) ? std::string(start, end) : "";
}

inline ImU32 ParseHexColor(const std::string& hex) {
    // Default to white with full alpha
    unsigned int color = 0xFFFFFFFF;

    if (hex[0] == '#') {
        std::stringstream ss;
        ss << std::hex << hex.substr(1);  // Skip '#'
        ss >> color;

        if (hex.length() == 7) {
            // #RRGGBB -> Append full alpha (255)
            color = (color << 8) | 0xFF;
        }
    }

    // Corrected byte order for IM_COL32
    return IM_COL32(
            (color >> 24) & 0xFF,  // Red
            (color >> 16) & 0xFF,  // Green
            (color >> 8) & 0xFF,   // Blue
            color & 0xFF           // Alpha
    );
}

inline static bool pasteTextureFromClipboard(std::shared_ptr<sf::Texture> texture, bool addAlphaChannel = true, bool removeMagenta = true) {
    if (!OpenClipboard(nullptr)) return false;

    HBITMAP hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
    if (hBitmap) {
        BITMAP bmp;
        GetObject(hBitmap, sizeof(BITMAP), &bmp);

        sf::Image image {sf::Vector2u(bmp.bmWidth, bmp.bmHeight), sf::Color::Transparent};

        HDC hdc = CreateCompatibleDC(nullptr);
        SelectObject(hdc, hBitmap);

        for (int y = 0; y < bmp.bmHeight; ++y) {
            for (int x = 0; x < bmp.bmWidth; ++x) {
                COLORREF color = GetPixel(hdc, x, y);
                image.setPixel(sf::Vector2u(x, y), sf::Color(GetRValue(color), GetGValue(color), GetBValue(color), 255));
            }
        }
        DeleteDC(hdc);

        // Ensure 32-bit RGBA
        if(addAlphaChannel) {
            ensureAlphaChannel(image);
        }
        if(removeMagenta) {
            imageRemoveMagenta(image);
        }

        texture->loadFromImage(image);
        CloseClipboard();
        return true;
    }

    CloseClipboard();
    return false;
}

// Ensures the image has an alpha channel (RGBA instead of RGB)
inline static void ensureAlphaChannel(sf::Image& image) {
    sf::Vector2u size = image.getSize();
    sf::Image temp {sf::Vector2u(size.x, size.y), sf::Color::Transparent}; // Create a 32-bit transparent image

    for (unsigned int y = 0; y < size.y; ++y) {
        for (unsigned int x = 0; x < size.x; ++x) {
            sf::Color pixel = image.getPixel({x, y});
            temp.setPixel(sf::Vector2u(x, y), sf::Color(pixel.r, pixel.g, pixel.b, 255)); // Ensure full opacity
        }
    }

    image = temp;
}

// Replaces magenta (255,0,255) with transparency
inline static void imageRemoveMagenta(sf::Image& image) {
    sf::Vector2u size = image.getSize();
    for (unsigned int y = 0; y < size.y; ++y) {
        for (unsigned int x = 0; x < size.x; ++x) {
            sf::Color pixel = image.getPixel({x, y});
            if (pixel.r == 255 && pixel.g == 0 && pixel.b == 255) { // Magenta color
                image.setPixel(sf::Vector2u(x, y), sf::Color(0, 0, 0, 0)); // Fully transparent
            }
        }
    }
}

// Copies an SFML texture to the clipboard
inline static bool copyTextureToClipboard(const sf::Texture& texture) {
    sf::Image image = texture.copyToImage();
    sf::Vector2u size = image.getSize();

    BITMAPINFOHEADER bi = {};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = size.x;
    bi.biHeight = -static_cast<LONG>(size.y);
    bi.biPlanes = 1;
    bi.biBitCount = 32; // 32-bit BGRX
    bi.biCompression = BI_RGB;
    bi.biSizeImage = size.x * size.y * 4;

    // Allocate memory for pixels
    std::vector<uint8_t> dibData(size.x * size.y * 4);
    uint8_t* bmpPixels = dibData.data();

    // Conversion RGBA (SFML) -> BGRX (Windows BMP)
    const uint8_t* pixels = image.getPixelsPtr();
    for (unsigned int i = 0; i < size.x * size.y; ++i) {
        bmpPixels[i * 4 + 0] = pixels[i * 4 + 2]; // Blue
        bmpPixels[i * 4 + 1] = pixels[i * 4 + 1]; // Green
        bmpPixels[i * 4 + 2] = pixels[i * 4 + 0]; // Red
        bmpPixels[i * 4 + 3] = 255;               // Alpha (ignorowane)
    }

    // Create HBITMAP with DIB data
    HDC hdc = GetDC(nullptr);
    HBITMAP hBitmap = CreateDIBitmap(
            hdc,
            &bi,
            CBM_INIT,
            dibData.data(),
            reinterpret_cast<BITMAPINFO*>(&bi),
            DIB_RGB_COLORS
    );
    ReleaseDC(nullptr, hdc);

    if (!hBitmap) {
        return false;
    }

    // Copy HBITMAP to clipboard
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();
        SetClipboardData(CF_BITMAP, hBitmap);
        CloseClipboard();
        DeleteObject(hBitmap); // Clipboard stores value, so we delete only when error
        return true;
    }

    DeleteObject(hBitmap);
    return false;
}

inline static bool copyItemTypeToClipboard(const ItemType& item) {
    // Serialize to memory stream
    std::stringstream stream;
    Items::serializeItemType(stream, item);
    std::string dataStr = stream.str();
    std::vector<uint8_t> data(dataStr.begin(), dataStr.end());

    // Prepend data size header
    size_t dataSize = data.size();
    std::vector<uint8_t> finalData;
    finalData.reserve(sizeof(dataSize) + dataSize);
    finalData.insert(finalData.end(), reinterpret_cast<uint8_t*>(&dataSize), reinterpret_cast<uint8_t*>(&dataSize) + sizeof(dataSize));
    finalData.insert(finalData.end(), data.begin(), data.end());

    // Copy to clipboard
    if (!OpenClipboard(nullptr)) return false;

    // Allocate global memory for the serialized data
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, finalData.size());
    if (!hGlobal) {
        CloseClipboard();
        return false;
    }

    // Lock memory and copy the data
    void* pData = GlobalLock(hGlobal);
    if (!pData) {
        GlobalFree(hGlobal);
        CloseClipboard();
        return false;
    }

    memcpy(pData, finalData.data(), finalData.size());  // Copy serialized data to clipboard
    GlobalUnlock(hGlobal);  // Unlock memory

    // Register clipboard format once
    static UINT format = RegisterClipboardFormat("ItemTypeBinary");

    // Empty clipboard and set new data
    EmptyClipboard();
    SetClipboardData(format, hGlobal);

    CloseClipboard();
    return true;
}

inline static bool pasteItemTypeFromClipboard(ItemType& itemType) {
    if (!OpenClipboard(nullptr)) return false;

    static UINT format = RegisterClipboardFormat("ItemTypeBinary");
    HANDLE hData = GetClipboardData(format);
    if (!hData) {
        CloseClipboard();
        return false;
    }

    void *pData = GlobalLock(hData);
    if (!pData) {
        CloseClipboard();
        return false;
    }

    size_t dataSize = GlobalSize(hData);
    std::vector<uint8_t> finalData(dataSize);
    memcpy(finalData.data(), pData, dataSize);
    GlobalUnlock(hData);
    CloseClipboard();

    // Validate size header
    if (dataSize < sizeof(size_t)) return false;
    size_t serializedSize;
    memcpy(&serializedSize, finalData.data(), sizeof(serializedSize));
    if (dataSize != sizeof(serializedSize) + serializedSize) return false;

    // Deserialize from memory stream
    std::string dataStr(finalData.begin() + sizeof(serializedSize), finalData.end());
    std::stringstream stream(dataStr);
    Items::deserializeItemType(stream, itemType);

    return true;
}

/**
 * @brief Helper method for graying out ImGui widget
 *
 * Pushes either gray or ImGui's blue colors onto the stack.
 * !CAUTION!: This method doesn't pop out the colors, and you need to do that on your own.
 * ImGui::PopStyleColor(colorsCount);
 *
 * @param predicate when true = grayed out
 * @return count of colors pushed onto the stack (later used to pop)
 */
inline static int pushImGuiGray(bool predicate) {
    if (predicate) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.6f)); // Grayed-out color
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.6f, 0.6f, 0.6f)); // Grayed-out hover color
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 0.6f)); // Grayed-out active color
    } else {
        ImGuiStyle &style = ImGui::GetStyle();
        ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, style.Colors[ImGuiCol_ButtonHovered]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, style.Colors[ImGuiCol_ButtonActive]);
    }

    // Update when you push more colors
    return 3;
}

inline static std::vector<std::string> getImageExtensions() {
    return {
            "bmp",
            "png",
            "tga",
            "jpg",
            "gif",
            "psd",
            "hdr",
            "pic",
            "pnm"
    };
}

inline std::string getExtensionFromPath(const std::string& filePath) {
    std::filesystem::path path(filePath);
    std::string ext = path.extension().string(); // e.g., ".jpg"

    if (!ext.empty() && ext[0] == '.')
        ext.erase(0, 1); // Remove leading dot

    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

inline std::string cleanPathIntoFolderPath(const std::string& path) {
    // Find last slash (both Windows '\' and Unix '/' supported)
    size_t lastSlash = path.find_last_of("\\/");
    if (lastSlash == std::string::npos) {
        return "";
    }

    // Extract last component (file or folder name)
    std::string lastPart = path.substr(lastSlash + 1);

    // Check if lastPart contains a dot ('.') indicating extension
    size_t dotPos = lastPart.find_last_of('.');
    if (dotPos != std::string::npos && dotPos != 0) {
        // There is an extension, so return path up to last slash only (folder)
        return path.substr(0, lastSlash);
    }

    // No extension detected, assume it's a folder, return as is
    return path;
}