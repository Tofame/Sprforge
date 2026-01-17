#include "DatReaderWriter.h"
#include "../Misc/Warninger.h"
#include "../Things/EffectType.h"
#include "../Things/Effects.h"
#include "../Things/ItemType.h"
#include "../Things/Items.h"
#include "../Things/MissileType.h"
#include "../Things/Missiles.h"
#include "../Things/OutfitType.h"
#include "../Things/Outfits.h"
#include <fmt/core.h>

DatReader::DatReader() = default;
DatReader::~DatReader() = default;

DatReadResult DatReader::readDatFile(const std::string& filePath, const ClientVersion* forcedVersion, bool extended,
									 bool improvedAnimations, bool frameGroups) {
	DatReadResult result;

	std::ifstream file(filePath, std::ios::binary);
	if (!file.is_open()) {
		result.errorMessage = "Failed to open file: " + filePath;
		return result;
	}

	// Read header
	if (!readHeader(file)) {
		result.errorMessage = "Failed to read dat header";
		return result;
	}
	result.header = m_header;

	// Detect or use forced version
	if (forcedVersion) {
		m_protocolVersion = forcedVersion->getProtocolVersion();
		m_clientVersion = forcedVersion->value;
		m_extended = extended || forcedVersion->isExtended();
		m_improvedAnimations = improvedAnimations || forcedVersion->hasImprovedAnimations();
		m_frameGroups = frameGroups || forcedVersion->hasFrameGroups();
		result.detectedVersion = *forcedVersion;
	} else {
		// Try to auto-detect version from signature
		auto detected = detectVersion(m_header.signature);
		if (detected.has_value()) {
			m_protocolVersion = detected->getProtocolVersion();
			m_clientVersion = detected->value;
			m_extended = extended || detected->isExtended();
			m_improvedAnimations = improvedAnimations || detected->hasImprovedAnimations();
			m_frameGroups = frameGroups || detected->hasFrameGroups();
			result.detectedVersion = detected;
			fmt::print("Auto-detected version: {} (protocol {})\n", detected->valueStr,
					   static_cast<int>(m_protocolVersion));
		} else {
			// Default to protocol 5 (8.60-9.86) with user settings
			m_protocolVersion = ProtocolVersion::PROTOCOL_860_986;
			m_clientVersion = 860;
			m_extended = extended;
			m_improvedAnimations = improvedAnimations;
			m_frameGroups = frameGroups;
			fmt::print("Could not detect version from signature 0x{:08X}, using defaults\n", m_header.signature);
		}
	}

	fmt::print("Loading dat: {} items, {} outfits, {} effects, {} missiles\n", m_header.itemsCount,
			   m_header.outfitsCount, m_header.effectsCount, m_header.missilesCount);
	fmt::print("Settings: extended={}, improvedAnimations={}, frameGroups={}, protocol={}\n", m_extended,
			   m_improvedAnimations, m_frameGroups, static_cast<int>(m_protocolVersion));

	// Load all categories
	if (!loadItems(file)) {
		result.errorMessage = "Failed to load items";
		return result;
	}

	if (!loadOutfits(file)) {
		result.errorMessage = "Failed to load outfits";
		return result;
	}

	if (!loadEffects(file)) {
		result.errorMessage = "Failed to load effects";
		return result;
	}

	if (!loadMissiles(file)) {
		result.errorMessage = "Failed to load missiles";
		return result;
	}

	// Check if we read everything correctly
	if (file.peek() != EOF && file.good()) {
		auto remaining = file.tellg();
		file.seekg(0, std::ios::end);
		auto total = file.tellg();
		fmt::print("Warning: {} bytes remaining in file after parsing\n", total - remaining);
	}

	result.success = true;
	return result;
}

bool DatReader::readHeader(std::ifstream& file) {
	file.read(reinterpret_cast<char*>(&m_header.signature), sizeof(m_header.signature));
	file.read(reinterpret_cast<char*>(&m_header.itemsCount), sizeof(m_header.itemsCount));
	file.read(reinterpret_cast<char*>(&m_header.outfitsCount), sizeof(m_header.outfitsCount));
	file.read(reinterpret_cast<char*>(&m_header.effectsCount), sizeof(m_header.effectsCount));
	file.read(reinterpret_cast<char*>(&m_header.missilesCount), sizeof(m_header.missilesCount));
	return file.good();
}

std::optional<ClientVersion> DatReader::detectVersion(uint32_t signature) {
	// Check version storage first for exact dat signature match
	const auto& versions = VersionStorage::getInstance().getVersions();
	for (const auto& ver : versions) {
		if (ver.datSignature == signature) {
			return ver;
		}
	}
	return std::nullopt;
}

bool DatReader::loadItems(std::ifstream& file) {
	// Item IDs start from 100
	uint32_t itemsToLoad = (m_header.itemsCount >= 100) ? (m_header.itemsCount - 100 + 1) : 0;

	for (uint32_t i = 0; i < itemsToLoad; ++i) {
		auto item = std::make_shared<ItemType>();
		uint32_t actualId = i + 100;

		if (!readItemProperties(file, item)) {
			Warninger::sendErrorMsg("DatReader::loadItems",
									"Failed to read properties for item " + std::to_string(actualId));
			// Still push empty item to maintain count
			Items::pushItemType(item);
			continue;
		}

		auto thingPtr = std::static_pointer_cast<ThingType>(item);
		if (!readTexturePatterns(file, thingPtr, ThingCategory::ITEM)) {
			Warninger::sendErrorMsg("DatReader::loadItems",
									"Failed to read texture patterns for item " + std::to_string(actualId));
		}

		Items::pushItemType(item);
	}

	return true;
}

bool DatReader::loadOutfits(std::ifstream& file) {
	for (uint32_t i = 1; i <= m_header.outfitsCount; ++i) {
		auto outfit = std::make_shared<OutfitType>();
		outfit->category = ThingCategory::OUTFIT;

		auto thingPtr = std::static_pointer_cast<ThingType>(outfit);
		if (!readThingProperties(file, thingPtr)) {
			Warninger::sendErrorMsg("DatReader::loadOutfits",
									"Failed to read properties for outfit " + std::to_string(i));
		}

		if (!readTexturePatterns(file, thingPtr, ThingCategory::OUTFIT)) {
			Warninger::sendErrorMsg("DatReader::loadOutfits",
									"Failed to read texture patterns for outfit " + std::to_string(i));
		}

		Outfits::pushOutfitType(outfit);
	}
	return true;
}

bool DatReader::loadEffects(std::ifstream& file) {
	for (uint32_t i = 1; i <= m_header.effectsCount; ++i) {
		auto effect = std::make_shared<EffectType>();
		effect->category = ThingCategory::EFFECT;

		auto thingPtr = std::static_pointer_cast<ThingType>(effect);
		if (!readThingProperties(file, thingPtr)) {
			Warninger::sendErrorMsg("DatReader::loadEffects",
									"Failed to read properties for effect " + std::to_string(i));
		}

		if (!readTexturePatterns(file, thingPtr, ThingCategory::EFFECT)) {
			Warninger::sendErrorMsg("DatReader::loadEffects",
									"Failed to read texture patterns for effect " + std::to_string(i));
		}

		Effects::pushEffectType(effect);
	}
	return true;
}

bool DatReader::loadMissiles(std::ifstream& file) {
	for (uint32_t i = 1; i <= m_header.missilesCount; ++i) {
		auto missile = std::make_shared<MissileType>();
		missile->category = ThingCategory::MISSILE;

		auto thingPtr = std::static_pointer_cast<ThingType>(missile);
		if (!readThingProperties(file, thingPtr)) {
			Warninger::sendErrorMsg("DatReader::loadMissiles",
									"Failed to read properties for missile " + std::to_string(i));
		}

		if (!readTexturePatterns(file, thingPtr, ThingCategory::MISSILE)) {
			Warninger::sendErrorMsg("DatReader::loadMissiles",
									"Failed to read texture patterns for missile " + std::to_string(i));
		}

		Missiles::pushMissileType(missile);
	}
	return true;
}

bool DatReader::readItemProperties(std::ifstream& file, std::shared_ptr<ItemType>& item) {
	uint8_t flag;

	while (true) {
		file.read(reinterpret_cast<char*>(&flag), 1);
		if (!file.good() || file.eof()) {
			return false;
		}

		if (flag == MetadataFlags::LAST_FLAG) {
			break;
		}

		// Handle flags based on protocol version
		switch (flag) {
		case MetadataFlags::GROUND: {
			item->setFlag(IS_GROUND, true);
			file.read(reinterpret_cast<char*>(&item->speed), sizeof(item->speed));
			break;
		}
		case MetadataFlags::GROUND_BORDER:
			item->itemCategory = GROUND_BORDER;
			break;
		case MetadataFlags::ON_BOTTOM:
			item->itemCategory = BOTTOM;
			break;
		case MetadataFlags::ON_TOP:
			item->itemCategory = TOP;
			break;
		case MetadataFlags::CONTAINER:
			item->setFlag(IS_CONTAINER, true);
			break;
		case MetadataFlags::STACKABLE:
			item->setFlag(STACKABLE, true);
			break;
		case MetadataFlags::FORCE_USE:
			item->setFlag(FORCE_USE, true);
			break;
		case MetadataFlags::MULTI_USE:
			item->setFlag(MULTI_USE, true);
			break;
		default:
			// Handle protocol-specific flags
			if (!skipFlagData(file, flag)) {
				return false;
			}
			break;
		}
	}
	return true;
}

bool DatReader::readThingProperties(std::ifstream& file, std::shared_ptr<ThingType>& thing) {
	uint8_t flag;

	while (true) {
		file.read(reinterpret_cast<char*>(&flag), 1);
		if (!file.good() || file.eof()) {
			return false;
		}

		if (flag == MetadataFlags::LAST_FLAG) {
			break;
		}

		// Skip any flag data for non-item types
		if (!skipFlagData(file, flag)) {
			return false;
		}
	}
	return true;
}

bool DatReader::skipFlagData(std::ifstream& file, uint8_t flag) {
	// Skip data associated with flags based on protocol version
	// This handles the different flag layouts between protocols

	// Common flags that need data skipping
	switch (flag) {
	case MetadataFlags::GROUND: {
		// Speed (2 bytes)
		file.seekg(2, std::ios::cur);
		break;
	}
	default: {
		// Protocol-specific handling
		uint8_t writableFlag = MetadataFlags::getWritableFlag(m_protocolVersion);
		uint8_t writableOnceFlag = MetadataFlags::getWritableOnceFlag(m_protocolVersion);
		uint8_t hasLightFlag = MetadataFlags::getHasLightFlag(m_protocolVersion);
		uint8_t hasOffsetFlag = MetadataFlags::getHasOffsetFlag(m_protocolVersion);
		uint8_t hasElevationFlag = MetadataFlags::getHasElevationFlag(m_protocolVersion);
		uint8_t miniMapFlag = MetadataFlags::getMiniMapFlag(m_protocolVersion);
		uint8_t lensHelpFlag = MetadataFlags::getLensHelpFlag(m_protocolVersion);
		uint8_t clothFlag = MetadataFlags::getClothFlag(m_protocolVersion);
		uint8_t marketItemFlag = MetadataFlags::getMarketItemFlag(m_protocolVersion);
		uint8_t defaultActionFlag = MetadataFlags::getDefaultActionFlag(m_protocolVersion);

		if (flag == writableFlag || flag == writableOnceFlag) {
			// Max chars (2 bytes)
			file.seekg(2, std::ios::cur);
		} else if (flag == hasLightFlag) {
			// Light intensity + color (4 bytes: 2+2)
			file.seekg(4, std::ios::cur);
		} else if (flag == hasOffsetFlag) {
			// Offset X and Y (4 bytes: 2+2)
			file.seekg(4, std::ios::cur);
		} else if (flag == hasElevationFlag) {
			// Height (2 bytes)
			file.seekg(2, std::ios::cur);
		} else if (flag == miniMapFlag) {
			// Minimap color (2 bytes)
			file.seekg(2, std::ios::cur);
		} else if (flag == lensHelpFlag) {
			// Lens help option (2 bytes)
			file.seekg(2, std::ios::cur);
		} else if (flag == clothFlag) {
			// Cloth slot (2 bytes)
			file.seekg(2, std::ios::cur);
		} else if (flag == marketItemFlag) {
			// Market data: category(2) + tradeAs(2) + showAs(2) + nameLen(2) + name + vocation(2) + level(2)
			file.seekg(6, std::ios::cur); // category + tradeAs + showAs
			uint16_t nameLength;
			file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
			if (nameLength > 0 && nameLength < 256) {
				file.seekg(nameLength, std::ios::cur);
			}
			file.seekg(4, std::ios::cur); // vocation + level
		} else if (flag == defaultActionFlag && m_protocolVersion == ProtocolVersion::PROTOCOL_1010_PLUS) {
			// Default action (2 bytes)
			file.seekg(2, std::ios::cur);
		}
		// Flags without data: GROUND_BORDER, ON_BOTTOM, ON_TOP, CONTAINER, STACKABLE, etc.
		break;
	}
	}

	return file.good();
}

bool DatReader::readTexturePatterns(std::ifstream& file, std::shared_ptr<ThingType>& thing, ThingCategory category) {
	// For outfits with frame groups (10.57+), read multiple frame groups
	uint8_t groupCount = 1;
	if (m_frameGroups && category == ThingCategory::OUTFIT) {
		file.read(reinterpret_cast<char*>(&groupCount), 1);
		if (!file.good())
			return false;
	}

	// For now, we read the first (or only) frame group into the thing's properties
	for (uint8_t g = 0; g < groupCount; ++g) {
		if (m_frameGroups && category == ThingCategory::OUTFIT) {
			// Read frame group type byte (but we don't store it for now)
			uint8_t groupType;
			file.read(reinterpret_cast<char*>(&groupType), 1);
		}

		FrameGroup group;
		if (!readFrameGroup(file, group)) {
			return false;
		}

		// Store first group's data in thing (for backward compatibility)
		if (g == 0) {
			thing->width = group.width;
			thing->height = group.height;
			thing->layers = group.layers;
			thing->patternX = group.patternX;
			thing->patternY = group.patternY;
			thing->patternZ = group.patternZ;
			thing->animationsFrames = group.frames;
			thing->textureIdsVector = group.spriteIndex;
		}
	}

	return true;
}

bool DatReader::readFrameGroup(std::ifstream& file, FrameGroup& group) {
	file.read(reinterpret_cast<char*>(&group.width), 1);
	file.read(reinterpret_cast<char*>(&group.height), 1);
	if (!file.good())
		return false;

	// Read exact size if width or height > 1
	if (group.width > 1 || group.height > 1) {
		file.read(reinterpret_cast<char*>(&group.exactSize), 1);
	} else {
		group.exactSize = 32; // Default
	}

	file.read(reinterpret_cast<char*>(&group.layers), 1);
	file.read(reinterpret_cast<char*>(&group.patternX), 1);
	file.read(reinterpret_cast<char*>(&group.patternY), 1);
	file.read(reinterpret_cast<char*>(&group.patternZ), 1);
	file.read(reinterpret_cast<char*>(&group.frames), 1);
	if (!file.good())
		return false;

	// Validate values - ensure minimum of 1
	if (group.width == 0)
		group.width = 1;
	if (group.height == 0)
		group.height = 1;
	if (group.layers == 0)
		group.layers = 1;
	if (group.patternX == 0)
		group.patternX = 1;
	if (group.patternY == 0)
		group.patternY = 1;
	if (group.patternZ == 0)
		group.patternZ = 1;
	if (group.frames == 0)
		group.frames = 1;

	// Read animation data if this has multiple frames
	if (group.frames > 1) {
		group.isAnimation = true;
		if (!readFrameDurations(file, group)) {
			return false;
		}
	}

	// Read sprite indices
	uint32_t totalSprites = group.getTotalSprites();
	group.spriteIndex.resize(totalSprites, 0);

	for (uint32_t i = 0; i < totalSprites; ++i) {
		if (m_extended) {
			// 32-bit sprite ID
			uint32_t spriteId;
			file.read(reinterpret_cast<char*>(&spriteId), 4);
			group.spriteIndex[i] = spriteId;
		} else {
			// 16-bit sprite ID
			uint16_t spriteId;
			file.read(reinterpret_cast<char*>(&spriteId), 2);
			group.spriteIndex[i] = spriteId;
		}
	}

	return file.good();
}

bool DatReader::readFrameDurations(std::ifstream& file, FrameGroup& group) {
	if (m_improvedAnimations) {
		// Read animation mode, loop count, start frame
		file.read(reinterpret_cast<char*>(&group.animationMode), 1);
		file.read(reinterpret_cast<char*>(&group.loopCount), sizeof(group.loopCount)); // int32
		file.read(reinterpret_cast<char*>(&group.startFrame), 1);					   // int8

		// Read frame durations
		group.frameDurations.resize(group.frames);
		for (uint8_t f = 0; f < group.frames; ++f) {
			uint32_t minimum, maximum;
			file.read(reinterpret_cast<char*>(&minimum), sizeof(minimum));
			file.read(reinterpret_cast<char*>(&maximum), sizeof(maximum));
			group.frameDurations[f] = FrameDuration(minimum, maximum);
		}
	} else {
		// No improved animations - use default durations
		group.frameDurations.resize(group.frames);
		for (uint8_t f = 0; f < group.frames; ++f) {
			group.frameDurations[f] = FrameDuration(DefaultDurations::ITEM, DefaultDurations::ITEM);
		}
	}

	return file.good();
}

// DatWriter implementation
DatWriter::DatWriter() = default;
DatWriter::~DatWriter() = default;

void DatWriter::setVersion(const ClientVersion& version) {
	m_signature = version.datSignature;
	m_protocolVersion = version.getProtocolVersion();
	m_clientVersion = version.value;
	m_extended = version.isExtended();
	m_improvedAnimations = version.hasImprovedAnimations();
	m_frameGroups = version.hasFrameGroups();
}

bool DatWriter::writeDatFile(const std::string& filePath) {
	std::ofstream file(filePath, std::ios::binary);
	if (!file.is_open()) {
		Warninger::sendErrorMsg("DatWriter::writeDatFile", "Failed to open file for writing: " + filePath);
		return false;
	}

	if (!writeHeader(file)) {
		return false;
	}

	if (!writeItems(file)) {
		return false;
	}

	if (!writeOutfits(file)) {
		return false;
	}

	if (!writeEffects(file)) {
		return false;
	}

	if (!writeMissiles(file)) {
		return false;
	}

	file.close();
	fmt::print("Successfully wrote dat file: {}\n", filePath);
	return true;
}

bool DatWriter::writeHeader(std::ofstream& file) {
	// Write signature
	file.write(reinterpret_cast<const char*>(&m_signature), sizeof(m_signature));

	// Write counts
	uint16_t itemsCount = static_cast<uint16_t>(Items::getItemTypesCount());
	uint16_t outfitsCount = static_cast<uint16_t>(Outfits::getOutfitTypesCount());
	uint16_t effectsCount = static_cast<uint16_t>(Effects::getEffectTypesCount());
	uint16_t missilesCount = static_cast<uint16_t>(Missiles::getMissileTypesCount());

	// Ensure minimum item count of 100
	if (itemsCount < 100)
		itemsCount = 100;

	file.write(reinterpret_cast<const char*>(&itemsCount), sizeof(itemsCount));
	file.write(reinterpret_cast<const char*>(&outfitsCount), sizeof(outfitsCount));
	file.write(reinterpret_cast<const char*>(&effectsCount), sizeof(effectsCount));
	file.write(reinterpret_cast<const char*>(&missilesCount), sizeof(missilesCount));

	return file.good();
}

bool DatWriter::writeItems(std::ofstream& file) {
	uint16_t itemCount = static_cast<uint16_t>(Items::getItemTypesCount());
	if (itemCount < 100)
		itemCount = 100;

	for (uint16_t id = 100; id <= itemCount; ++id) {
		auto item = Items::getItemType(id);
		if (!item) {
			// Write empty item (just terminator)
			uint8_t terminator = MetadataFlags::LAST_FLAG;
			file.write(reinterpret_cast<const char*>(&terminator), 1);
			continue;
		}

		if (!writeItemProperties(file, item)) {
			return false;
		}

		auto thingPtr = std::static_pointer_cast<ThingType>(item);
		if (!writeTexturePatterns(file, thingPtr, ThingCategory::ITEM)) {
			return false;
		}
	}

	return true;
}

bool DatWriter::writeOutfits(std::ofstream& file) {
	uint16_t count = static_cast<uint16_t>(Outfits::getOutfitTypesCount());

	for (uint16_t i = 0; i < count; ++i) {
		auto outfit = Outfits::getOutfitType(i);
		if (!outfit) {
			outfit = std::make_shared<OutfitType>();
		}

		// Write terminator for properties (outfits typically have no flags)
		uint8_t terminator = MetadataFlags::LAST_FLAG;
		file.write(reinterpret_cast<const char*>(&terminator), 1);

		auto thingPtr = std::static_pointer_cast<ThingType>(outfit);
		if (!writeTexturePatterns(file, thingPtr, ThingCategory::OUTFIT)) {
			return false;
		}
	}

	return true;
}

bool DatWriter::writeEffects(std::ofstream& file) {
	uint16_t count = static_cast<uint16_t>(Effects::getEffectTypesCount());

	for (uint16_t i = 0; i < count; ++i) {
		auto effect = Effects::getEffectType(i);
		if (!effect) {
			effect = std::make_shared<EffectType>();
		}

		uint8_t terminator = MetadataFlags::LAST_FLAG;
		file.write(reinterpret_cast<const char*>(&terminator), 1);

		auto thingPtr = std::static_pointer_cast<ThingType>(effect);
		if (!writeTexturePatterns(file, thingPtr, ThingCategory::EFFECT)) {
			return false;
		}
	}

	return true;
}

bool DatWriter::writeMissiles(std::ofstream& file) {
	uint16_t count = static_cast<uint16_t>(Missiles::getMissileTypesCount());

	for (uint16_t i = 0; i < count; ++i) {
		auto missile = Missiles::getMissileType(i);
		if (!missile) {
			missile = std::make_shared<MissileType>();
		}

		uint8_t terminator = MetadataFlags::LAST_FLAG;
		file.write(reinterpret_cast<const char*>(&terminator), 1);

		auto thingPtr = std::static_pointer_cast<ThingType>(missile);
		if (!writeTexturePatterns(file, thingPtr, ThingCategory::MISSILE)) {
			return false;
		}
	}

	return true;
}

bool DatWriter::writeItemProperties(std::ofstream& file, const std::shared_ptr<ItemType>& item) {
	// Write flags using protocol-specific values
	if (item->hasFlag(IS_GROUND)) {
		uint8_t flag = MetadataFlags::GROUND;
		file.write(reinterpret_cast<const char*>(&flag), 1);
		file.write(reinterpret_cast<const char*>(&item->speed), sizeof(item->speed));
	}

	if (item->itemCategory == GROUND_BORDER) {
		uint8_t flag = MetadataFlags::GROUND_BORDER;
		file.write(reinterpret_cast<const char*>(&flag), 1);
	}

	if (item->itemCategory == BOTTOM) {
		uint8_t flag = MetadataFlags::ON_BOTTOM;
		file.write(reinterpret_cast<const char*>(&flag), 1);
	}

	if (item->itemCategory == TOP) {
		uint8_t flag = MetadataFlags::ON_TOP;
		file.write(reinterpret_cast<const char*>(&flag), 1);
	}

	if (item->hasFlag(IS_CONTAINER)) {
		uint8_t flag = MetadataFlags::CONTAINER;
		file.write(reinterpret_cast<const char*>(&flag), 1);
	}

	if (item->hasFlag(STACKABLE)) {
		uint8_t flag = MetadataFlags::STACKABLE;
		file.write(reinterpret_cast<const char*>(&flag), 1);
	}

	if (item->hasFlag(FORCE_USE)) {
		uint8_t flag = MetadataFlags::FORCE_USE;
		file.write(reinterpret_cast<const char*>(&flag), 1);
	}

	if (item->hasFlag(MULTI_USE)) {
		uint8_t flag = MetadataFlags::MULTI_USE;
		file.write(reinterpret_cast<const char*>(&flag), 1);
	}

	// Protocol-specific flags
	if (item->hasFlag(UNPASSABLE)) {
		uint8_t flag = MetadataFlags::getUnpassableFlag(m_protocolVersion);
		file.write(reinterpret_cast<const char*>(&flag), 1);
	}

	if (item->hasFlag(UNMOVABLE)) {
		uint8_t flag = MetadataFlags::getUnmoveableFlag(m_protocolVersion);
		file.write(reinterpret_cast<const char*>(&flag), 1);
	}

	if (item->hasFlag(BLOCK_MISSILE)) {
		uint8_t flag = MetadataFlags::getBlockMissileFlag(m_protocolVersion);
		file.write(reinterpret_cast<const char*>(&flag), 1);
	}

	if (item->hasFlag(PICKUPABLE)) {
		uint8_t flag = MetadataFlags::getPickupableFlag(m_protocolVersion);
		file.write(reinterpret_cast<const char*>(&flag), 1);
	}

	if (item->hasLight()) {
		uint8_t flag = MetadataFlags::getHasLightFlag(m_protocolVersion);
		file.write(reinterpret_cast<const char*>(&flag), 1);
		file.write(reinterpret_cast<const char*>(&item->lightBlock.lightIntensity),
				   sizeof(item->lightBlock.lightIntensity));
		file.write(reinterpret_cast<const char*>(&item->lightBlock.lightColor), sizeof(item->lightBlock.lightColor));
	}

	if (item->minimapColor > 0) {
		uint8_t flag = MetadataFlags::getMiniMapFlag(m_protocolVersion);
		file.write(reinterpret_cast<const char*>(&flag), 1);
		file.write(reinterpret_cast<const char*>(&item->minimapColor), sizeof(item->minimapColor));
	}

	// Write terminator
	uint8_t terminator = MetadataFlags::LAST_FLAG;
	file.write(reinterpret_cast<const char*>(&terminator), 1);

	return file.good();
}

bool DatWriter::writeTexturePatterns(std::ofstream& file, const std::shared_ptr<ThingType>& thing,
									 ThingCategory category) {
	// For outfits with frame groups (10.57+)
	if (m_frameGroups && category == ThingCategory::OUTFIT) {
		uint8_t groupCount = 1; // For now, we only support 1 group
		file.write(reinterpret_cast<const char*>(&groupCount), 1);

		// Write group type
		uint8_t groupType = 0;
		file.write(reinterpret_cast<const char*>(&groupType), 1);
	}

	// Create a frame group from thing data
	FrameGroup group;
	group.width = thing->width;
	group.height = thing->height;
	group.layers = thing->layers;
	group.patternX = thing->patternX;
	group.patternY = thing->patternY;
	group.patternZ = thing->patternZ;
	group.frames = thing->animationsFrames;
	group.spriteIndex = thing->textureIdsVector;
	group.isAnimation = thing->animationsFrames > 1;

	// Set default durations if animated
	if (group.isAnimation) {
		group.frameDurations.resize(group.frames);
		for (uint8_t f = 0; f < group.frames; ++f) {
			group.frameDurations[f] = FrameDuration(100, 100);
		}
	}

	return writeFrameGroup(file, group);
}

bool DatWriter::writeFrameGroup(std::ofstream& file, const FrameGroup& group) {
	file.write(reinterpret_cast<const char*>(&group.width), 1);
	file.write(reinterpret_cast<const char*>(&group.height), 1);

	// Write exact size if width or height > 1
	if (group.width > 1 || group.height > 1) {
		file.write(reinterpret_cast<const char*>(&group.exactSize), 1);
	}

	file.write(reinterpret_cast<const char*>(&group.layers), 1);
	file.write(reinterpret_cast<const char*>(&group.patternX), 1);
	file.write(reinterpret_cast<const char*>(&group.patternY), 1);
	file.write(reinterpret_cast<const char*>(&group.patternZ), 1);
	file.write(reinterpret_cast<const char*>(&group.frames), 1);

	// Write animation data if this has multiple frames
	if (group.frames > 1) {
		if (!writeFrameDurations(file, group)) {
			return false;
		}
	}

	// Write sprite indices
	uint32_t totalSprites = group.getTotalSprites();

	for (uint32_t i = 0; i < totalSprites; ++i) {
		uint32_t spriteId = (i < group.spriteIndex.size()) ? group.spriteIndex[i] : 0;

		if (m_extended) {
			file.write(reinterpret_cast<const char*>(&spriteId), 4);
		} else {
			uint16_t spriteId16 = static_cast<uint16_t>(spriteId);
			file.write(reinterpret_cast<const char*>(&spriteId16), 2);
		}
	}

	return file.good();
}

bool DatWriter::writeFrameDurations(std::ofstream& file, const FrameGroup& group) {
	if (m_improvedAnimations) {
		// Write animation mode, loop count, start frame
		file.write(reinterpret_cast<const char*>(&group.animationMode), 1);
		file.write(reinterpret_cast<const char*>(&group.loopCount), sizeof(group.loopCount));
		file.write(reinterpret_cast<const char*>(&group.startFrame), 1);

		// Write frame durations
		for (uint8_t f = 0; f < group.frames; ++f) {
			FrameDuration dur = (f < group.frameDurations.size()) ? group.frameDurations[f] : FrameDuration(100, 100);
			file.write(reinterpret_cast<const char*>(&dur.minimum), sizeof(dur.minimum));
			file.write(reinterpret_cast<const char*>(&dur.maximum), sizeof(dur.maximum));
		}
	}
	// If not improved animations, no duration data to write

	return file.good();
}
