#include "Effects.h"
#include "../Misc/Warninger.h"
#include "../Misc/definitions.h"

std::vector<std::shared_ptr<EffectType>> Effects::effectTypes = std::vector<std::shared_ptr<EffectType>>();

bool Effects::isValidEffectTypeIndex(uint32_t id) {
    return id < effectTypes.size() && effectTypes.at(id) != nullptr;
}

void Effects::pushEffectType(std::shared_ptr<EffectType> eType) {
    effectTypes.push_back(std::move(eType));
}

void Effects::removeEffectType(uint32_t id) {
    if (!isValidEffectTypeIndex(id)) {
        return;
    }
    effectTypes[id].reset();
    if (id == effectTypes.size() - 1) {
        effectTypes.pop_back();
    }
}

std::shared_ptr<EffectType> Effects::getEffectType(uint32_t id) {
    if (!isValidEffectTypeIndex(id)) {
        return dollEffectType;
    }
    return effectTypes.at(id);
}

bool Effects::replaceEffectType(uint32_t effectTypeId, std::shared_ptr<EffectType> newEffectType) {
    if (isValidEffectTypeIndex(effectTypeId)) {
        effectTypes[effectTypeId] = std::move(newEffectType);
        return true;
    } else {
        Warninger::sendWarning(FUNC_NAME, "Invalid EffectType ID " + std::to_string(effectTypeId));
    }
    return false;
}

