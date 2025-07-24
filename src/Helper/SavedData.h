#pragma once

#include <string>
#include "toml++/toml.h"
#include <iostream>

class SavedData {
public:
    static SavedData* getInstance() {
        if(instance_ == nullptr) {
            instance_ = new SavedData();
            instance_->loadData("appData.toml");
        }

        return instance_;
    }

    toml::table& getData() {
        return data_;
    }

    void setDataString(const std::string& key, const std::string& value);
    const std::string getDataString(const std::string& key);

    void setDataBool(const std::string& key, const bool& value);
    bool getDataBool(const std::string& key);

    void saveData(const std::string& filename = "appData.toml");
private:
    static SavedData* instance_;
    toml::table data_;

    SavedData() = default;
    // Prevent copying and assignment
    SavedData(const SavedData&) = delete;
    SavedData& operator=(const SavedData&) = delete;

    void loadData(const std::string& filename);
};