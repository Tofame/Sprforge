#include "Items.h"
#include "../Misc/Warninger.h"
#include "../Misc/definitions.h"
#include <toml++/toml.h>
#include <iostream>
#include <cstdint>

std::vector<std::shared_ptr<ItemType>> Items::itemTypes = std::vector<std::shared_ptr<ItemType>>();

bool Items::isValidItemTypeIndex(uint32_t id) {
    return id < itemTypes.size() && itemTypes.at(id) != nullptr;
}

void Items::pushItemType(std::shared_ptr<ItemType> iType) {
    itemTypes.push_back(std::move(iType));
}

void Items::removeItemType(uint32_t id) {
    if (!isValidItemTypeIndex(id)) {
        return;
    }

    itemTypes[id].reset();

    // Shrink vector if the last element was removed
    if (id == itemTypes.size() - 1) {
        itemTypes.pop_back();
    }
}

std::shared_ptr<ItemType> Items::getItemType(uint32_t id) {
    if (!isValidItemTypeIndex(id)) {
        return dollItemType;
    }
    return itemTypes.at(id);
}

bool Items::replaceItemType(uint32_t itemTypeId, std::shared_ptr<ItemType> newItemType) {
    if (isValidItemTypeIndex(itemTypeId)) {
        itemTypes[itemTypeId] = std::move(newItemType);
        return true;
    } else {
        Warninger::sendWarning(FUNC_NAME, "Invalid ItemType ID " + std::to_string(itemTypeId));
    }
    return false;
}

uint32_t Items::getItemIdByName(const std::string& name) {
    if (!name.empty()) {
        uint32_t i = 2; // skip 0 (undefined) and 1 (air/empty)
        while (i < itemTypes.size() && itemTypes[i] != nullptr) {
            const auto& iType = itemTypes[i];
            if (!strcasecmp(name.c_str(), iType->name.c_str()))
                return i;
            i++;
        }
    }
    return 0;
}

void Items::exportItemToml(const std::string& filePath, int itemId) {    // Serialize and write to file
    std::ofstream file(filePath);
    if (!file.is_open()) {
        Warninger::sendWarning(FUNC_NAME, "Failed to open file: " + filePath);
        return;
    }

    auto itemType = *getItemType(itemId);

    // Create a TOML table
    toml::table itemData;
    itemData.insert("id", itemId);
    itemData.insert("category", static_cast<int>(itemType.category));

    itemData.insert("isGround", itemType.hasFlag(IS_GROUND));
    itemData.insert("speed", itemType.speed);

    itemData.insert("stackable", itemType.hasFlag(STACKABLE));
    itemData.insert("forceUse", itemType.hasFlag(FORCE_USE));
    itemData.insert("unmovable", itemType.hasFlag(UNMOVABLE));
    itemData.insert("pickupable", itemType.hasFlag(PICKUPABLE));
    itemData.insert("unpassable", itemType.hasFlag(UNPASSABLE));
    itemData.insert("isContainer", itemType.hasFlag(IS_CONTAINER));
    itemData.insert("multiUse", itemType.hasFlag(MULTI_USE));

    itemData.insert("width", itemType.width);
    itemData.insert("height", itemType.height);
    itemData.insert("animationsFrames", itemType.animationsFrames);

    // Convert textureIdsVector to a TOML array
    toml::array textureIds;
    for (const auto& textureId : itemType.textureIdsVector) {
        textureIds.push_back(textureId);
    }
    itemData.insert("textureIdsVector", std::move(textureIds));

    file << itemData;
    file.close();
}

void Items::exportItemItf(const std::string &filePath, int itemId) {
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        Warninger::sendWarning(FUNC_NAME, "Failed to open file: " + filePath);
        return;
    }

    auto itemType = *getItemType(itemId);
    serializeItemType(file, itemType);
    file.close();
}

bool Items::importItemToml(const std::string& filePath) {
    // Open and parse TOML file
    toml::table itemData;
    try {
        itemData = toml::parse_file(filePath);
    } catch (const std::exception& e) {
        Warninger::sendWarning(FUNC_NAME, "Failed to parse TOML file: " + filePath);
        return false;
    }

    // Create new shared_ptr and push
    auto itemTypePtr = std::make_shared<ItemType>();
    Items::pushItemType(itemTypePtr);
    ItemType& itemType = *itemTypePtr;

    // Assign values from TOML
    itemType.category = static_cast<ItemCategory_t>(itemData["category"].value_or(0));

    itemType.setFlag(IS_GROUND, itemData["isGround"].value_or(false));
    itemType.speed = itemData["speed"].value_or(0);

    itemType.setFlag(IS_GROUND, itemData["isGround"].value_or(false));
    itemType.setFlag(STACKABLE, itemData["stackable"].value_or(false));
    itemType.setFlag(FORCE_USE, itemData["forceUse"].value_or(false));
    itemType.setFlag(UNPASSABLE, itemData["unpassable"].value_or(false));
    itemType.setFlag(PICKUPABLE, itemData["pickupable"].value_or(false));
    itemType.setFlag(IS_CONTAINER, itemData["isContainer"].value_or(false));
    itemType.setFlag(MULTI_USE, itemData["multiUse"].value_or(false));

    itemType.width = itemData["width"].value_or(1);
    itemType.height = itemData["height"].value_or(1);
    itemType.animationsFrames = itemData["animationsFrames"].value_or(1);

    // Read texture IDs
    itemType.textureIdsVector.clear();
    if (itemData.contains("textureIdsVector") && itemData["textureIdsVector"].is_array()) {
        for (const auto& textureId : *itemData["textureIdsVector"].as_array()) {
            if (textureId.is_integer()) {
                itemType.textureIdsVector.push_back(textureId.value<int>().value_or(0));
            }
        }
    }

    return true;
}

bool Items::importItemItf(const std::string &filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        Warninger::sendWarning(FUNC_NAME, "Failed to open file: " + filePath);
        return false;
    }

    // Ensure the item exists
    auto itemTypePtr = std::make_shared<ItemType>();
    Items::pushItemType(itemTypePtr);
    deserializeItemType(file, *itemTypePtr);

    file.close();
    return true;
}

void Items::serializeItemType(std::ostream &stream, const ItemType &itemType) {
    // Write primitive members
    stream.write(reinterpret_cast<const char*>(&itemType.category), sizeof(itemType.category));

    auto flags = itemType.getAllFlags();
    stream.write(reinterpret_cast<const char*>(&flags), sizeof(flags));

    if(itemType.hasFlag(IS_GROUND)) {
        stream.write(reinterpret_cast<const char *>(&itemType.speed), sizeof(itemType.speed));
    }

    stream.write(reinterpret_cast<const char*>(&itemType.width), sizeof(itemType.width));
    stream.write(reinterpret_cast<const char*>(&itemType.height), sizeof(itemType.height));
    stream.write(reinterpret_cast<const char*>(&itemType.animationsFrames), sizeof(itemType.animationsFrames));

    // Write textureIdsVector
    uint16_t textureCount = static_cast<uint16_t>(itemType.textureIdsVector.size());
    stream.write(reinterpret_cast<const char*>(&textureCount), sizeof(textureCount));
    if (!itemType.textureIdsVector.empty()) {
        stream.write(reinterpret_cast<const char*>(itemType.textureIdsVector.data()),
                     textureCount * sizeof(itemType.textureIdsVector[0]));
    }
}

void Items::deserializeItemType(std::istream& stream, ItemType& itemType) {
    // Read primitive data members
    stream.read(reinterpret_cast<char*>(&itemType.category), sizeof(itemType.category));

    auto flags = itemType.getAllFlags();
    stream.read(reinterpret_cast<char*>(&flags), sizeof(flags));
    itemType.setAllFlags(flags);

    if(itemType.hasFlag(IS_GROUND)) {
        stream.read(reinterpret_cast<char *>(&itemType.speed), sizeof(itemType.speed));
    }

    stream.read(reinterpret_cast<char*>(&itemType.width), sizeof(itemType.width));
    stream.read(reinterpret_cast<char*>(&itemType.height), sizeof(itemType.height));
    stream.read(reinterpret_cast<char*>(&itemType.animationsFrames), sizeof(itemType.animationsFrames));

    // Read textureIdsVector
    uint16_t textureCount;
    stream.read(reinterpret_cast<char*>(&textureCount), sizeof(textureCount));

    itemType.textureIdsVector.resize(textureCount);
    if (textureCount > 0) {
        stream.read(reinterpret_cast<char*>(itemType.textureIdsVector.data()),
                    textureCount * sizeof(itemType.textureIdsVector[0]));
    }
}