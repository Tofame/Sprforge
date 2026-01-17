#pragma once

#include "../Things/ThingCategory.h"
#include "FrameGroup.h"
#include "MetadataFlags.h"
#include "Version.h"
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <vector>


// Forward declarations
class ThingType;
class ItemType;
class OutfitType;
class EffectType;
class MissileType;

// Structure to hold parsed dat file header info
struct DatHeader {
	uint32_t signature = 0;
	uint16_t itemsCount = 0;
	uint16_t outfitsCount = 0;
	uint16_t effectsCount = 0;
	uint16_t missilesCount = 0;
};

// Result of reading a dat file
struct DatReadResult {
	bool success = false;
	std::string errorMessage;
	DatHeader header;
	std::optional<ClientVersion> detectedVersion;
};

// Main class for reading .dat files with protocol version awareness
class DatReader {
public:
	DatReader();
	~DatReader();

	// Main entry point for reading a dat file
	// Will auto-detect the protocol version based on signature and file structure
	DatReadResult readDatFile(const std::string& filePath, const ClientVersion* forcedVersion = nullptr,
							  bool extended = false, bool improvedAnimations = false, bool frameGroups = false);

	// Getters for result data
	[[nodiscard]] const DatHeader& getHeader() const { return m_header; }
	[[nodiscard]] ProtocolVersion getProtocolVersion() const { return m_protocolVersion; }
	[[nodiscard]] bool isExtended() const { return m_extended; }
	[[nodiscard]] bool hasImprovedAnimations() const { return m_improvedAnimations; }
	[[nodiscard]] bool hasFrameGroups() const { return m_frameGroups; }

private:
	// Read the dat file header
	bool readHeader(std::ifstream& file);

	// Detect protocol version from signature
	std::optional<ClientVersion> detectVersion(uint32_t signature);

	// Read item properties based on protocol version
	bool readItemProperties(std::ifstream& file, std::shared_ptr<ItemType>& item);

	// Read thing properties based on protocol version (for outfits, effects, missiles)
	bool readThingProperties(std::ifstream& file, std::shared_ptr<ThingType>& thing);

	// Read texture patterns (dimensions, animations, sprite IDs)
	bool readTexturePatterns(std::ifstream& file, std::shared_ptr<ThingType>& thing, ThingCategory category);

	// Read frame group data (for outfits in 10.57+)
	bool readFrameGroup(std::ifstream& file, FrameGroup& group);

	// Read frame durations for animated objects
	bool readFrameDurations(std::ifstream& file, FrameGroup& group);

	// Helper to skip data of unknown flags
	bool skipFlagData(std::ifstream& file, uint8_t flag);

	// Processing functions for each category
	bool loadItems(std::ifstream& file);
	bool loadOutfits(std::ifstream& file);
	bool loadEffects(std::ifstream& file);
	bool loadMissiles(std::ifstream& file);

	// Member variables
	DatHeader m_header;
	ProtocolVersion m_protocolVersion = ProtocolVersion::PROTOCOL_860_986;
	bool m_extended = false;
	bool m_improvedAnimations = false;
	bool m_frameGroups = false;
	uint32_t m_clientVersion = 860; // Default to 8.60
};

// Main class for writing .dat files with protocol version awareness
class DatWriter {
public:
	DatWriter();
	~DatWriter();

	// Set the target protocol version for writing
	void setVersion(const ClientVersion& version);
	void setExtended(bool extended) { m_extended = extended; }
	void setImprovedAnimations(bool improved) { m_improvedAnimations = improved; }
	void setFrameGroups(bool frameGroups) { m_frameGroups = frameGroups; }

	// Main entry point for writing a dat file
	bool writeDatFile(const std::string& filePath);

private:
	// Write the dat file header
	bool writeHeader(std::ofstream& file);

	// Write item properties based on protocol version
	bool writeItemProperties(std::ofstream& file, const std::shared_ptr<ItemType>& item);

	// Write thing properties based on protocol version
	bool writeThingProperties(std::ofstream& file, const std::shared_ptr<ThingType>& thing);

	// Write texture patterns
	bool writeTexturePatterns(std::ofstream& file, const std::shared_ptr<ThingType>& thing, ThingCategory category);

	// Write frame group data
	bool writeFrameGroup(std::ofstream& file, const FrameGroup& group);

	// Write frame durations
	bool writeFrameDurations(std::ofstream& file, const FrameGroup& group);

	// Processing functions for each category
	bool writeItems(std::ofstream& file);
	bool writeOutfits(std::ofstream& file);
	bool writeEffects(std::ofstream& file);
	bool writeMissiles(std::ofstream& file);

	// Member variables
	uint32_t m_signature = 0;
	ProtocolVersion m_protocolVersion = ProtocolVersion::PROTOCOL_860_986;
	bool m_extended = false;
	bool m_improvedAnimations = false;
	bool m_frameGroups = false;
	uint32_t m_clientVersion = 860;
};
