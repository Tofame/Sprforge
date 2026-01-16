#include "Missiles.h"
#include "../Misc/Warninger.h"
#include "../Misc/definitions.h"

std::vector<std::shared_ptr<MissileType>> Missiles::missileTypes = std::vector<std::shared_ptr<MissileType>>();

bool Missiles::isValidMissileTypeIndex(uint32_t id) {
	return id < missileTypes.size() && missileTypes.at(id) != nullptr;
}

void Missiles::pushMissileType(std::shared_ptr<MissileType> mType) {
	missileTypes.push_back(std::move(mType));
}

void Missiles::removeMissileType(uint32_t id) {
	if (!isValidMissileTypeIndex(id)) {
		return;
	}
	missileTypes[id].reset();
	if (id == missileTypes.size() - 1) {
		missileTypes.pop_back();
	}
}

std::shared_ptr<MissileType> Missiles::getMissileType(uint32_t id) {
	if (!isValidMissileTypeIndex(id)) {
		return dollMissileType;
	}
	return missileTypes.at(id);
}

bool Missiles::replaceMissileType(uint32_t missileTypeId, std::shared_ptr<MissileType> newMissileType) {
	if (isValidMissileTypeIndex(missileTypeId)) {
		missileTypes[missileTypeId] = std::move(newMissileType);
		return true;
	} else {
		Warninger::sendWarning(FUNC_NAME, "Invalid MissileType ID " + std::to_string(missileTypeId));
	}
	return false;
}
