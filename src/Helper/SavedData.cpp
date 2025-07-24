#include "SavedData.h"
#include "../Misc/definitions.h"
#include "../Misc/Warninger.h"

SavedData* SavedData::instance_ = nullptr;

void SavedData::loadData(const std::string &filename) {
    try {
        std::ifstream file(filename);
        if (file.is_open()) {
            data_ = toml::parse(file);
        } else {
            Warninger::sendErrorMsg(FUNC_NAME, "Unable to open file" + filename);
        }
    } catch (const toml::parse_error& err) {
        std::cerr << "Error loading TOML file: " << err.description() << std::endl;
    }
}

void SavedData::saveData(const std::string& filename) {
    SavedData* savedData = SavedData::getInstance();
    toml::table& data = savedData->getData();

    std::ofstream outFile(filename);
    if (outFile.is_open()) {
        outFile << data;
        outFile.close();
    } else {
        Warninger::sendErrorMsg(FUNC_NAME, "Unable to open file" + filename);
    }
}

const std::string SavedData::getDataString(const std::string &key) {
    if (auto value = SavedData::getInstance()->getData()[key]) {
        if (auto strValue = value.value<std::string>()) {
            return *strValue;
        } else {
            Warninger::sendWarning(FUNC_NAME, "Value for key" + key + " is not a string.");
        }
    }

    return std::string("");
}

void SavedData::setDataString(const std::string& key, const std::string& value) {
    SavedData* savedData = SavedData::getInstance();
    toml::table& data = savedData->getData();
    data.insert_or_assign(key, value);
}

bool SavedData::getDataBool(const std::string &key) {
    if (auto value = SavedData::getInstance()->getData()[key]) {
        if (auto valueFound = value.value<bool>()) {
            return *valueFound;
        } else {
            Warninger::sendWarning(FUNC_NAME, "Value for key" + key + " is not a string.");
        }
    }

    return false;
}

void SavedData::setDataBool(const std::string &key, const bool &value) {
    SavedData* savedData = SavedData::getInstance();
    toml::table& data = savedData->getData();
    data.insert_or_assign(key, value);
}