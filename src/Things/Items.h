#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

#include <toml++/toml.h>
#include "ItemType.h"

class Items
{
public:
    Items() = default;
    virtual ~Items() = default;

    // Basic Methods
    static bool isValidItemTypeIndex(uint32_t id);
    static std::shared_ptr<ItemType> getItemType(uint32_t id);

    static void pushItemType(std::shared_ptr<ItemType> iType);
    static void removeItemType(uint32_t id);
    static bool replaceItemType(uint32_t id, std::shared_ptr<ItemType> newItemType);

    std::shared_ptr<ItemType> operator[](uint32_t id) const { return itemTypes.at(id); }
    static uint32_t getItemTypesCount() { return static_cast<uint32_t>(itemTypes.size()); }
    static const std::vector<std::shared_ptr<ItemType>>& getItemTypes() { return itemTypes; }

    // Utility Methods
    static uint32_t getItemIdByName(const std::string& name);

    // Export Methods
    static void exportItemToml(const std::string& filePath, int itemId);
    static void exportItemItf(const std::string& filePath, int itemId);

    static bool importItemToml(const std::string& filePath);
    static bool importItemItf(const std::string& filePath);

    static void serializeItemType(std::ostream& stream, const ItemType& itemType);
    static void deserializeItemType(std::istream& stream, ItemType& itemType);

    static void clearItemTypes() {
        itemTypes.clear();
    }

    static inline std::shared_ptr<ItemType> dollItemType = std::make_shared<ItemType>();
private:
    static std::vector<std::shared_ptr<ItemType>> itemTypes;
};