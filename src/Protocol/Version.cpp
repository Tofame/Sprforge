#include "Version.h"

void VersionStorage::initialize() {
	if (m_initialized)
		return;

	// These are from Object Builder's versions.xml
	// Each version has: value, string, datSignature (hex), sprSignature (hex), otbVersion
	// clang-format off
	m_versions = {
		// 7.1x - 7.3x (Protocol 1)
		{710, "7.10", 0x3DFF4B2A, 0x3DFF4AEB, 0},
		{730, "7.30", 0x411A6233, 0x411A6279, 0},
		
		// 7.4x - 7.5x (Protocol 2)
		{740, "7.40", 0x41BF619C, 0x41B9EA86, 1},
		{750, "7.50", 0x42F81973, 0x42F81949, 1},
		
		// 7.55 - 7.72 (Protocol 3)
		{755, "7.55", 0x437B2B8F, 0x434F9CDE, 2},
		{760, "7.60", 0x439D5A33, 0x439852BE, 3},
		{770, "7.70", 0x439D5A33, 0x439852BE, 3},
		
		// 7.8x - 8.54 (Protocol 4)
		{780, "7.80", 0x44CE4743, 0x44CE4206, 4},
		{790, "7.90", 0x457D854E, 0x457957C8, 5},
		{792, "7.92", 0x459E7B73, 0x45880FE8, 6},
		{800, "8.00", 0x467FD7E6, 0x467F9E74, 7},
		{810, "8.10", 0x475D3747, 0x475D0B01, 8},
		{811, "8.11", 0x47F60E37, 0x47EBB9B2, 9},
		{820, "8.20", 0x486905AA, 0x4868ECC9, 10},
		{830, "8.30", 0x48DA1FB6, 0x48C8E712, 11},
		{840, "8.40", 0x493D607A, 0x493D4E7C, 12},
		{841, "8.41", 0x49B7CC19, 0x49B140EA, 13},
		{842, "8.42", 0x49C233C9, 0x49B140EA, 14},
		{850, "8.50 v1", 0x4A49C5EB, 0x4A44FD4E, 15},
		{850, "8.50 v2", 0x4A4CC0DC, 0x4A44FD4E, 15},
		{850, "8.50 v3", 0x4AE97492, 0x4ACB5230, 15},
		{852, "8.52", 0x4A4CC0DC, 0x4A44FD4E, 0},
		{853, "8.53", 0x4AE97492, 0x4ACB5230, 0},
		{854, "8.54 v1", 0x4B1E2CAA, 0x4B1E2C87, 16},
		{854, "8.54 v2", 0x4B0D46A9, 0x4B0D3AFF, 16},
		{854, "8.54 v3", 0x4B28B89E, 0x4B1E2C87, 17},
		
		// 8.55 - 9.86 (Protocol 5: 8.60+ format, same flags as current implementation)
		{855, "8.55", 0x4B98FF53, 0x4B913871, 18},
		{860, "8.60 v1", 0x4C28B721, 0x4C220594, 19},
		{860, "8.60 v2", 0x4C2C7993, 0x4C220594, 20},
		{861, "8.61", 0x4C6A4CBC, 0x4C63F145, 21},
		{862, "8.62", 0x4C973450, 0x4C63F145, 22},
		{870, "8.70", 0x4CFE22C5, 0x4CFD078A, 23},
		{871, "8.71", 0x4D41979E, 0x4D3D65D0, 24},
		{872, "8.72", 0x4DAD1A1A, 0x4DAD1A32, 25},
		{900, "9.00", 0x4DBAA20B, 0x4DAD1A32, 27},
		{910, "9.10", 0x4E12DAFF, 0x4E12DB27, 28},
		{920, "9.20", 0x4E807C08, 0x4E807C23, 29},
		{940, "9.40", 0x4EE71DE5, 0x4EE71E06, 30},
		{944, "9.44 v0", 0x4F0EEFBB, 0x4F0EEFEF, 31},
		{944, "9.44 v1", 0x4F105168, 0x4F1051D7, 32},
		{944, "9.44 v2", 0x4F16C0D7, 0x4F1051D7, 33},
		{944, "9.44 v3", 0x4F3131CF, 0x4F3131F6, 34},
		{946, "9.46", 0x4F75B7AB, 0x4F5DCEF7, 35},
		{950, "9.50", 0x4F75B7AB, 0x4F75B7CD, 36},
		{952, "9.52", 0x4F857F6C, 0x4F857F8E, 37},
		{953, "9.53", 0x4FA11252, 0x4FA11282, 38},
		{954, "9.54", 0x4FD5956B, 0x4FD595B7, 39},
		{960, "9.60", 0x4FFA74CC, 0x4FFA74F9, 40},
		{961, "9.61", 0x50226F9D, 0x50226FBD, 41},
		{963, "9.63", 0x503CB933, 0x503CB954, 42},
		{970, "9.70", 0x5072A490, 0x5072A567, 43},
		{980, "9.80", 0x50C70674, 0x50C70753, 44},
		{981, "9.81", 0x50D1C5B6, 0x50D1C685, 45},
		{982, "9.82", 0x512CAD09, 0x512CAD68, 46},
		{983, "9.83", 0x51407B67, 0x51407BC7, 47},
		{985, "9.85", 0x51641A1B, 0x51641A84, 48},
		{986, "9.86", 0x5170E904, 0x5170E96F, 49},
		
		// 10.10+ (Protocol 6: NO_MOVE_ANIMATION flag added, shifts all flags after BLOCK_PATHFIND)
		{1010, "10.10", 0x51E3F8C3, 0x51E3F8E9, 50},
		{1020, "10.20", 0x5236F129, 0x5236F14F, 51},
		{1021, "10.21", 0x526A5068, 0x526A5090, 52},
		{1030, "10.30", 0x52A59036, 0x52A5905F, 53},
		{1031, "10.31", 0x52AED581, 0x52AED5A7, 54},
		{1032, "10.32", 0x52D8D0A9, 0x52D8D0CE, 0},
		{1034, "10.34", 0x52E74AB5, 0x52E74ADA, 0},
		{1035, "10.35", 0x52FDFC2C, 0x52FDFC54, 55},
		{1036, "10.36", 0x53159C7E, 0x53159CA9, 0},
		{1037, "10.37", 0x531EA82E, 0x531EA856, 0},
		{1038, "10.38", 0x5333C199, 0x5333C1C3, 0},
		{1039, "10.39", 0x535A50AD, 0x535A50D5, 0},
		{1040, "10.40", 0x5379984D, 0x53799876, 0},
		{1041, "10.41", 0x5383504E, 0x53835077, 0},
		{1050, "10.50", 0x53B6460E, 0x53B64639, 0},
		{1051, "10.51", 0x53C8CC17, 0x53C8CC3F, 0},
		{1052, "10.52", 0x53E898BD, 0x53E898E5, 0},
		{1053, "10.53", 0x53FAD76E, 0x53FAD799, 0},
		{1054, "10.54", 0x540D3A47, 0x53E898E5, 0},
		{1055, "10.55", 0x54128727, 0x54128755, 0},
		{1056, "10.56", 0x542143B0, 0x542143DE, 0},
		
		// 10.57+ adds frame groups for outfits
		{1057, "10.57", 0x542535F9, 0x54253627, 0},
		{1058, "10.58", 0x542D12E7, 0x542D1315, 0},
		{1059, "10.59", 0x5434084B, 0x54340879, 0},
		{1060, "10.60", 0x5448D9C7, 0x5448DA10, 0},
		{1061, "10.61", 0x5448D9C7, 0x5448DA10, 0},
		{1062, "10.62", 0x54622638, 0x54622667, 0},
		{1063, "10.63", 0x546B502A, 0x546B505E, 0},
		{1064, "10.64", 0x547F05BE, 0x547F0632, 0},
		{1070, "10.70", 0x5481BB97, 0x5481BC06, 0},
		{1071, "10.71", 0x334F, 0x548E9EFE, 0},
		{1072, "10.72", 0x3729, 0x54B37B99, 0},
		{1073, "10.73", 0x374D, 0x54BC95AE, 0},
		{1074, "10.74", 0x375E, 0x54C5FAB2, 0},
		{1075, "10.75", 0x3775, 0x54D85085, 0},
		{1076, "10.76", 0x37DF, 0x54F03CE9, 0},
		{1077, "10.77", 0x38DE, 0x5525213D, 0},
		{1090, "10.90", 0x3F26, 0x565EE171, 0},
		{1091, "10.91", 0x3F81, 0x56BC8198, 0},
		{1092, "10.92", 0x4086, 0x570742B8, 0},
		{1093, "10.93 test", 0x40FF, 0x57161DEA, 0},
		{1093, "10.93", 0x413F, 0x5726E657, 0},
		{1094, "10.94", 0x41E5, 0x57459D43, 0},
		{1095, "10.95", 0x41F3, 0x575A84BD, 0},
		{1098, "10.98", 0x42A3, 0x57BBD603, 0},
		{1099, "10.99", 0x4347, 0x57FF106B, 0},
		{1286, "12.86", 0x4A10, 0x59E48E02, 0},
	};
	// clang-format on

	// Build version strings for UI
	m_versionStrings.clear();
	m_versionStrings.reserve(m_versions.size());
	for (const auto& ver : m_versions) {
		m_versionStrings.push_back(ver.valueStr);
	}

	m_initialized = true;
}

std::optional<ClientVersion> VersionStorage::getBySignatures(uint32_t datSignature, uint32_t sprSignature) const {
	for (const auto& version : m_versions) {
		if (version.datSignature == datSignature && version.sprSignature == sprSignature) {
			return version;
		}
	}
	return std::nullopt;
}

std::optional<ClientVersion> VersionStorage::getByValue(uint32_t value) const {
	for (const auto& version : m_versions) {
		if (version.value == value) {
			return version;
		}
	}
	return std::nullopt;
}

std::optional<ClientVersion> VersionStorage::getByValueString(const std::string& valueStr) const {
	for (const auto& version : m_versions) {
		if (version.valueStr == valueStr) {
			return version;
		}
	}
	return std::nullopt;
}

void VersionStorage::addVersion(const ClientVersion& version) {
	m_versions.push_back(version);
	m_versionStrings.push_back(version.valueStr);
}
