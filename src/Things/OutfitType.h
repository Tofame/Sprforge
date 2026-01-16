#pragma once

#include "ThingType.h"

class OutfitType : public ThingType {
public:
	OutfitType();
	virtual ~OutfitType() = default;
	OutfitType(const OutfitType&) = default;
	OutfitType& operator=(const OutfitType&) = default;
};
