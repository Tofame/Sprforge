#include "Outfits.h"
#include "../Misc/Warninger.h"
#include "../Misc/definitions.h"

std::vector<std::shared_ptr<OutfitType>> Outfits::outfitTypes = std::vector<std::shared_ptr<OutfitType>>();

bool Outfits::isValidOutfitTypeIndex(uint32_t id) {
	return id < outfitTypes.size() && outfitTypes.at(id) != nullptr;
}

void Outfits::pushOutfitType(std::shared_ptr<OutfitType> oType) {
	outfitTypes.push_back(std::move(oType));
}

void Outfits::removeOutfitType(uint32_t id) {
	if (!isValidOutfitTypeIndex(id)) {
		return;
	}
	outfitTypes[id].reset();
	if (id == outfitTypes.size() - 1) {
		outfitTypes.pop_back();
	}
}

std::shared_ptr<OutfitType> Outfits::getOutfitType(uint32_t id) {
	if (!isValidOutfitTypeIndex(id)) {
		return dollOutfitType;
	}
	return outfitTypes.at(id);
}

bool Outfits::replaceOutfitType(uint32_t outfitTypeId, std::shared_ptr<OutfitType> newOutfitType) {
	if (isValidOutfitTypeIndex(outfitTypeId)) {
		outfitTypes[outfitTypeId] = std::move(newOutfitType);
		return true;
	} else {
		Warninger::sendWarning(FUNC_NAME, "Invalid OutfitType ID " + std::to_string(outfitTypeId));
	}
	return false;
}
