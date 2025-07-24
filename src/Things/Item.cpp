#include "Item.h"

Item* Item::CreateItem(const uint32_t itemTypeId, uint8_t count/* = 0*/, bool loadedFromMap/* = false*/)
{
    Item* newItem = nullptr;
    newItem = new Item(itemTypeId, count, loadedFromMap);

    return newItem;
}

Item::Item(const uint32_t itemTypeId, uint8_t amount/* = 0*/, bool loadedFromMap/* = false*/)
{
    this->itemTypeId = itemTypeId;
    this->loadedFromMap = false;

    setItemCount(1);
    auto const it = Items::getItemType(itemTypeId);

    if(it->hasFlag(STACKABLE)) {
        if(amount) {
            setItemCount(amount);
        }
    }
}