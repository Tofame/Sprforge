#pragma once

#include "ThingType.h"

class EffectType : public ThingType {
public:
	EffectType();
	virtual ~EffectType() = default;
	EffectType(const EffectType&) = default;
	EffectType& operator=(const EffectType&) = default;
};
