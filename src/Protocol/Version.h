#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Protocol version ranges for determining which reader/writer to use
// Based on Object Builder's MetadataReaderX implementations
enum class ProtocolVersion {
	PROTOCOL_710_730 = 1,  // MetadataReader1
	PROTOCOL_740_750 = 2,  // MetadataReader2
	PROTOCOL_755_772 = 3,  // MetadataReader3
	PROTOCOL_780_854 = 4,  // MetadataReader4
	PROTOCOL_860_986 = 5,  // MetadataReader5
	PROTOCOL_1010_PLUS = 6 // MetadataReader6
};

// Client version information structure
struct ClientVersion {
	uint32_t value = 0;	  // e.g., 860 for 8.60
	std::string valueStr; // e.g., "8.60"
	uint32_t datSignature = 0;
	uint32_t sprSignature = 0;
	uint32_t otbVersion = 0;

	// Auto-detected features based on version
	[[nodiscard]] bool isExtended() const { return value >= 960; }
	[[nodiscard]] bool hasImprovedAnimations() const { return value >= 1050; }
	[[nodiscard]] bool hasFrameGroups() const { return value >= 1057; }

	// Get the protocol version for this client version
	[[nodiscard]] ProtocolVersion getProtocolVersion() const {
		if (value <= 730)
			return ProtocolVersion::PROTOCOL_710_730;
		if (value <= 750)
			return ProtocolVersion::PROTOCOL_740_750;
		if (value <= 772)
			return ProtocolVersion::PROTOCOL_755_772;
		if (value <= 854)
			return ProtocolVersion::PROTOCOL_780_854;
		if (value <= 986)
			return ProtocolVersion::PROTOCOL_860_986;
		return ProtocolVersion::PROTOCOL_1010_PLUS;
	}

	bool operator==(const ClientVersion& other) const {
		return value == other.value && datSignature == other.datSignature && sprSignature == other.sprSignature;
	}
};

// Version storage - manages all known client versions
class VersionStorage {
public:
	static VersionStorage& getInstance() {
		static VersionStorage instance;
		return instance;
	}

	// Initialize with default versions
	void initialize();

	// Get version by signatures (auto-detection)
	std::optional<ClientVersion> getBySignatures(uint32_t datSignature, uint32_t sprSignature) const;

	// Get version by value (e.g., 860)
	std::optional<ClientVersion> getByValue(uint32_t value) const;

	// Get version by value string (e.g., "8.60")
	std::optional<ClientVersion> getByValueString(const std::string& valueStr) const;

	// Get all versions
	[[nodiscard]] const std::vector<ClientVersion>& getVersions() const { return m_versions; }

	// Get version strings for UI dropdowns
	[[nodiscard]] const std::vector<std::string>& getVersionStrings() const { return m_versionStrings; }

	// Add a custom version
	void addVersion(const ClientVersion& version);

private:
	VersionStorage() { initialize(); }
	~VersionStorage() = default;
	VersionStorage(const VersionStorage&) = delete;
	VersionStorage& operator=(const VersionStorage&) = delete;

	std::vector<ClientVersion> m_versions;
	std::vector<std::string> m_versionStrings;
	bool m_initialized = false;
};
