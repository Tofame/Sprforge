#pragma once
#include "OutfitType.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class Outfits {
public:
	Outfits() = default;
	virtual ~Outfits() = default;

	static bool isValidOutfitTypeIndex(uint32_t id);
	static std::shared_ptr<OutfitType> getOutfitType(uint32_t id);
	static void pushOutfitType(std::shared_ptr<OutfitType> oType);
	static void removeOutfitType(uint32_t id);
	static bool replaceOutfitType(uint32_t id, std::shared_ptr<OutfitType> newOutfitType);
	static uint32_t getOutfitTypesCount() { return static_cast<uint32_t>(outfitTypes.size()); }
	static const std::vector<std::shared_ptr<OutfitType>>& getOutfitTypes() { return outfitTypes; }
	static void clearOutfitTypes() { outfitTypes.clear(); }

	std::shared_ptr<OutfitType> operator[](uint32_t id) const { return outfitTypes.at(id); }

private:
	static std::vector<std::shared_ptr<OutfitType>> outfitTypes;
	static inline std::shared_ptr<OutfitType> dollOutfitType = std::make_shared<OutfitType>();
};
