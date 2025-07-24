#pragma once

#include <cstdint>
#include "Items.h"
#include "Thing.h"

class Item : virtual public Thing {
public:
    //Factory member to create item of right type based on type
    static Item* CreateItem(uint32_t itemTypeId, uint8_t count = 0, bool loadedFromMap = false);

    // Constructor for items
    explicit Item(uint32_t itemTypeId, uint8_t count = 0, bool loadedFromMap = false);
    virtual ~Item() = default;

    Item* getItem() override {return this;}
    [[nodiscard]] virtual const Item* getItem() const {return this;}

    [[nodiscard]] uint8_t getItemCount() const {return count;}
    void setItemCount(uint8_t n) {count = n;}

    [[nodiscard]] uint16_t getItemTypeId() const { return itemTypeId; }
    [[nodiscard]] bool isLoadedFromMap() const { return loadedFromMap; }
protected:
    uint16_t itemTypeId;
    uint8_t count;

    bool loadedFromMap;
};