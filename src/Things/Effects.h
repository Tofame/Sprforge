#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include "EffectType.h"

class Effects
{
public:
    Effects() = default;
    virtual ~Effects() = default;

    static bool isValidEffectTypeIndex(uint32_t id);
    static std::shared_ptr<EffectType> getEffectType(uint32_t id);
    static void pushEffectType(std::shared_ptr<EffectType> eType);
    static void removeEffectType(uint32_t id);
    static bool replaceEffectType(uint32_t id, std::shared_ptr<EffectType> newEffectType);
    static uint32_t getEffectTypesCount() { return static_cast<uint32_t>(effectTypes.size()); }
    static const std::vector<std::shared_ptr<EffectType>>& getEffectTypes() { return effectTypes; }
    static void clearEffectTypes() { effectTypes.clear(); }

    std::shared_ptr<EffectType> operator[](uint32_t id) const { return effectTypes.at(id); }

private:
    static std::vector<std::shared_ptr<EffectType>> effectTypes;
    static inline std::shared_ptr<EffectType> dollEffectType = std::make_shared<EffectType>();
};

