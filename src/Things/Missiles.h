#pragma once
#include "MissileType.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class Missiles {
public:
	Missiles() = default;
	virtual ~Missiles() = default;

	static bool isValidMissileTypeIndex(uint32_t id);
	static std::shared_ptr<MissileType> getMissileType(uint32_t id);
	static void pushMissileType(std::shared_ptr<MissileType> mType);
	static void removeMissileType(uint32_t id);
	static bool replaceMissileType(uint32_t id, std::shared_ptr<MissileType> newMissileType);
	static uint32_t getMissileTypesCount() { return static_cast<uint32_t>(missileTypes.size()); }
	static const std::vector<std::shared_ptr<MissileType>>& getMissileTypes() { return missileTypes; }
	static void clearMissileTypes() { missileTypes.clear(); }

	std::shared_ptr<MissileType> operator[](uint32_t id) const { return missileTypes.at(id); }

private:
	static std::vector<std::shared_ptr<MissileType>> missileTypes;
	static inline std::shared_ptr<MissileType> dollMissileType = std::make_shared<MissileType>();
};
