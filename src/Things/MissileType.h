#pragma once

#include "ThingType.h"

class MissileType : public ThingType {
public:
    MissileType();
    virtual ~MissileType() = default;
    MissileType(const MissileType&) = default;
    MissileType& operator=(const MissileType&) = default;
};

