#pragma once

#include "Version.h"
#include <cstdint>

// Protocol-specific metadata flags
// Based on Object Builder's MetadataFlags1-6 classes

// These flags define the binary format of .dat files.
// Different protocol versions have different flag values!
// The key difference between Protocol 5 (8.60-9.86) and Protocol 6 (10.10+):
// Protocol 6 adds NO_MOVE_ANIMATION (0x10), shifting all subsequent flags by +1

namespace MetadataFlags {

// Common flags shared across most protocols
constexpr uint8_t GROUND = 0x00;
constexpr uint8_t GROUND_BORDER = 0x01;
constexpr uint8_t ON_BOTTOM = 0x02;
constexpr uint8_t ON_TOP = 0x03;
constexpr uint8_t CONTAINER = 0x04;
constexpr uint8_t STACKABLE = 0x05;
constexpr uint8_t FORCE_USE = 0x06;
constexpr uint8_t MULTI_USE = 0x07;
constexpr uint8_t LAST_FLAG = 0xFF;

// Protocol 4 (7.80 - 8.54): Has HAS_CHARGES instead of WRITABLE at 0x08
namespace Protocol4 {
constexpr uint8_t HAS_CHARGES = 0x08;
constexpr uint8_t WRITABLE = 0x09;
constexpr uint8_t WRITABLE_ONCE = 0x0A;
constexpr uint8_t FLUID_CONTAINER = 0x0B;
constexpr uint8_t FLUID = 0x0C;
constexpr uint8_t UNPASSABLE = 0x0D;
constexpr uint8_t UNMOVEABLE = 0x0E;
constexpr uint8_t BLOCK_MISSILE = 0x0F;
constexpr uint8_t BLOCK_PATHFIND = 0x10;
constexpr uint8_t PICKUPABLE = 0x11;
constexpr uint8_t HANGABLE = 0x12;
constexpr uint8_t VERTICAL = 0x13;
constexpr uint8_t HORIZONTAL = 0x14;
constexpr uint8_t ROTATABLE = 0x15;
constexpr uint8_t HAS_LIGHT = 0x16;
constexpr uint8_t DONT_HIDE = 0x17;
constexpr uint8_t FLOOR_CHANGE = 0x18;
constexpr uint8_t HAS_OFFSET = 0x19;
constexpr uint8_t HAS_ELEVATION = 0x1A;
constexpr uint8_t LYING_OBJECT = 0x1B;
constexpr uint8_t ANIMATE_ALWAYS = 0x1C;
constexpr uint8_t MINI_MAP = 0x1D;
constexpr uint8_t LENS_HELP = 0x1E;
constexpr uint8_t FULL_GROUND = 0x1F;
constexpr uint8_t IGNORE_LOOK = 0x20;
} // namespace Protocol4

// Protocol 5 (8.55/8.60 - 9.86): The "classic" format most commonly used
namespace Protocol5 {
constexpr uint8_t WRITABLE = 0x08;
constexpr uint8_t WRITABLE_ONCE = 0x09;
constexpr uint8_t FLUID_CONTAINER = 0x0A;
constexpr uint8_t FLUID = 0x0B;
constexpr uint8_t UNPASSABLE = 0x0C;
constexpr uint8_t UNMOVEABLE = 0x0D;
constexpr uint8_t BLOCK_MISSILE = 0x0E;
constexpr uint8_t BLOCK_PATHFIND = 0x0F;
constexpr uint8_t PICKUPABLE = 0x10;
constexpr uint8_t HANGABLE = 0x11;
constexpr uint8_t VERTICAL = 0x12;
constexpr uint8_t HORIZONTAL = 0x13;
constexpr uint8_t ROTATABLE = 0x14;
constexpr uint8_t HAS_LIGHT = 0x15;
constexpr uint8_t DONT_HIDE = 0x16;
constexpr uint8_t TRANSLUCENT = 0x17;
constexpr uint8_t HAS_OFFSET = 0x18;
constexpr uint8_t HAS_ELEVATION = 0x19;
constexpr uint8_t LYING_OBJECT = 0x1A;
constexpr uint8_t ANIMATE_ALWAYS = 0x1B;
constexpr uint8_t MINI_MAP = 0x1C;
constexpr uint8_t LENS_HELP = 0x1D;
constexpr uint8_t FULL_GROUND = 0x1E;
constexpr uint8_t IGNORE_LOOK = 0x1F;
constexpr uint8_t CLOTH = 0x20;
constexpr uint8_t MARKET_ITEM = 0x21;
constexpr uint8_t WRAPPABLE = 0x24;
constexpr uint8_t UNWRAPPABLE = 0x25;
constexpr uint8_t BOTTOM_EFFECT = 0x26;
constexpr uint8_t DONT_CENTER_OUTFIT = 0x28;
} // namespace Protocol5

// Protocol 6 (10.10+): Adds NO_MOVE_ANIMATION, shifts all flags after BLOCK_PATHFIND by +1
namespace Protocol6 {
constexpr uint8_t WRITABLE = 0x08;
constexpr uint8_t WRITABLE_ONCE = 0x09;
constexpr uint8_t FLUID_CONTAINER = 0x0A;
constexpr uint8_t FLUID = 0x0B;
constexpr uint8_t UNPASSABLE = 0x0C;
constexpr uint8_t UNMOVEABLE = 0x0D;
constexpr uint8_t BLOCK_MISSILE = 0x0E;
constexpr uint8_t BLOCK_PATHFIND = 0x0F;
constexpr uint8_t NO_MOVE_ANIMATION = 0x10; // *** NEW IN PROTOCOL 6 ***
constexpr uint8_t PICKUPABLE = 0x11;		// shifted from 0x10
constexpr uint8_t HANGABLE = 0x12;			// shifted from 0x11
constexpr uint8_t VERTICAL = 0x13;			// shifted from 0x12
constexpr uint8_t HORIZONTAL = 0x14;		// shifted from 0x13
constexpr uint8_t ROTATABLE = 0x15;			// shifted from 0x14
constexpr uint8_t HAS_LIGHT = 0x16;			// shifted from 0x15
constexpr uint8_t DONT_HIDE = 0x17;			// shifted from 0x16
constexpr uint8_t TRANSLUCENT = 0x18;		// shifted from 0x17
constexpr uint8_t HAS_OFFSET = 0x19;		// shifted from 0x18
constexpr uint8_t HAS_ELEVATION = 0x1A;		// shifted from 0x19
constexpr uint8_t LYING_OBJECT = 0x1B;		// shifted from 0x1A
constexpr uint8_t ANIMATE_ALWAYS = 0x1C;	// shifted from 0x1B
constexpr uint8_t MINI_MAP = 0x1D;			// shifted from 0x1C
constexpr uint8_t LENS_HELP = 0x1E;			// shifted from 0x1D
constexpr uint8_t FULL_GROUND = 0x1F;		// shifted from 0x1E
constexpr uint8_t IGNORE_LOOK = 0x20;		// shifted from 0x1F
constexpr uint8_t CLOTH = 0x21;				// shifted from 0x20
constexpr uint8_t MARKET_ITEM = 0x22;		// shifted from 0x21
constexpr uint8_t DEFAULT_ACTION = 0x23;	// NEW in 10.10+
constexpr uint8_t WRAPPABLE = 0x24;
constexpr uint8_t UNWRAPPABLE = 0x25;
constexpr uint8_t BOTTOM_EFFECT = 0x26;
constexpr uint8_t USABLE = 0xFE;
} // namespace Protocol6

// Helper to get flag value based on protocol version
inline uint8_t getPickupableFlag(ProtocolVersion proto) {
	switch (proto) {
	case ProtocolVersion::PROTOCOL_780_854:
		return Protocol4::PICKUPABLE;
	case ProtocolVersion::PROTOCOL_860_986:
		return Protocol5::PICKUPABLE;
	case ProtocolVersion::PROTOCOL_1010_PLUS:
		return Protocol6::PICKUPABLE;
	default:
		return Protocol5::PICKUPABLE;
	}
}

inline uint8_t getUnpassableFlag(ProtocolVersion proto) {
	switch (proto) {
	case ProtocolVersion::PROTOCOL_780_854:
		return Protocol4::UNPASSABLE;
	case ProtocolVersion::PROTOCOL_860_986:
		return Protocol5::UNPASSABLE;
	case ProtocolVersion::PROTOCOL_1010_PLUS:
		return Protocol6::UNPASSABLE;
	default:
		return Protocol5::UNPASSABLE;
	}
}

inline uint8_t getUnmoveableFlag(ProtocolVersion proto) {
	switch (proto) {
	case ProtocolVersion::PROTOCOL_780_854:
		return Protocol4::UNMOVEABLE;
	case ProtocolVersion::PROTOCOL_860_986:
		return Protocol5::UNMOVEABLE;
	case ProtocolVersion::PROTOCOL_1010_PLUS:
		return Protocol6::UNMOVEABLE;
	default:
		return Protocol5::UNMOVEABLE;
	}
}

inline uint8_t getBlockMissileFlag(ProtocolVersion proto) {
	switch (proto) {
	case ProtocolVersion::PROTOCOL_780_854:
		return Protocol4::BLOCK_MISSILE;
	case ProtocolVersion::PROTOCOL_860_986:
		return Protocol5::BLOCK_MISSILE;
	case ProtocolVersion::PROTOCOL_1010_PLUS:
		return Protocol6::BLOCK_MISSILE;
	default:
		return Protocol5::BLOCK_MISSILE;
	}
}

inline uint8_t getHasLightFlag(ProtocolVersion proto) {
	switch (proto) {
	case ProtocolVersion::PROTOCOL_780_854:
		return Protocol4::HAS_LIGHT;
	case ProtocolVersion::PROTOCOL_860_986:
		return Protocol5::HAS_LIGHT;
	case ProtocolVersion::PROTOCOL_1010_PLUS:
		return Protocol6::HAS_LIGHT;
	default:
		return Protocol5::HAS_LIGHT;
	}
}

inline uint8_t getMiniMapFlag(ProtocolVersion proto) {
	switch (proto) {
	case ProtocolVersion::PROTOCOL_780_854:
		return Protocol4::MINI_MAP;
	case ProtocolVersion::PROTOCOL_860_986:
		return Protocol5::MINI_MAP;
	case ProtocolVersion::PROTOCOL_1010_PLUS:
		return Protocol6::MINI_MAP;
	default:
		return Protocol5::MINI_MAP;
	}
}

inline uint8_t getHasOffsetFlag(ProtocolVersion proto) {
	switch (proto) {
	case ProtocolVersion::PROTOCOL_780_854:
		return Protocol4::HAS_OFFSET;
	case ProtocolVersion::PROTOCOL_860_986:
		return Protocol5::HAS_OFFSET;
	case ProtocolVersion::PROTOCOL_1010_PLUS:
		return Protocol6::HAS_OFFSET;
	default:
		return Protocol5::HAS_OFFSET;
	}
}

inline uint8_t getHasElevationFlag(ProtocolVersion proto) {
	switch (proto) {
	case ProtocolVersion::PROTOCOL_780_854:
		return Protocol4::HAS_ELEVATION;
	case ProtocolVersion::PROTOCOL_860_986:
		return Protocol5::HAS_ELEVATION;
	case ProtocolVersion::PROTOCOL_1010_PLUS:
		return Protocol6::HAS_ELEVATION;
	default:
		return Protocol5::HAS_ELEVATION;
	}
}

inline uint8_t getLensHelpFlag(ProtocolVersion proto) {
	switch (proto) {
	case ProtocolVersion::PROTOCOL_780_854:
		return Protocol4::LENS_HELP;
	case ProtocolVersion::PROTOCOL_860_986:
		return Protocol5::LENS_HELP;
	case ProtocolVersion::PROTOCOL_1010_PLUS:
		return Protocol6::LENS_HELP;
	default:
		return Protocol5::LENS_HELP;
	}
}

inline uint8_t getClothFlag(ProtocolVersion proto) {
	switch (proto) {
	case ProtocolVersion::PROTOCOL_860_986:
		return Protocol5::CLOTH;
	case ProtocolVersion::PROTOCOL_1010_PLUS:
		return Protocol6::CLOTH;
	default:
		return Protocol5::CLOTH;
	}
}

inline uint8_t getMarketItemFlag(ProtocolVersion proto) {
	switch (proto) {
	case ProtocolVersion::PROTOCOL_860_986:
		return Protocol5::MARKET_ITEM;
	case ProtocolVersion::PROTOCOL_1010_PLUS:
		return Protocol6::MARKET_ITEM;
	default:
		return Protocol5::MARKET_ITEM;
	}
}

inline uint8_t getDefaultActionFlag(ProtocolVersion proto) {
	if (proto == ProtocolVersion::PROTOCOL_1010_PLUS) {
		return Protocol6::DEFAULT_ACTION;
	}
	return 0xFF; // Not supported in older protocols
}

inline uint8_t getWritableFlag(ProtocolVersion proto) {
	switch (proto) {
	case ProtocolVersion::PROTOCOL_780_854:
		return Protocol4::WRITABLE;
	case ProtocolVersion::PROTOCOL_860_986:
		return Protocol5::WRITABLE;
	case ProtocolVersion::PROTOCOL_1010_PLUS:
		return Protocol6::WRITABLE;
	default:
		return Protocol5::WRITABLE;
	}
}

inline uint8_t getWritableOnceFlag(ProtocolVersion proto) {
	switch (proto) {
	case ProtocolVersion::PROTOCOL_780_854:
		return Protocol4::WRITABLE_ONCE;
	case ProtocolVersion::PROTOCOL_860_986:
		return Protocol5::WRITABLE_ONCE;
	case ProtocolVersion::PROTOCOL_1010_PLUS:
		return Protocol6::WRITABLE_ONCE;
	default:
		return Protocol5::WRITABLE_ONCE;
	}
}

inline uint8_t getFluidContainerFlag(ProtocolVersion proto) {
	switch (proto) {
	case ProtocolVersion::PROTOCOL_780_854:
		return Protocol4::FLUID_CONTAINER;
	case ProtocolVersion::PROTOCOL_860_986:
		return Protocol5::FLUID_CONTAINER;
	case ProtocolVersion::PROTOCOL_1010_PLUS:
		return Protocol6::FLUID_CONTAINER;
	default:
		return Protocol5::FLUID_CONTAINER;
	}
}

inline uint8_t getFluidFlag(ProtocolVersion proto) {
	switch (proto) {
	case ProtocolVersion::PROTOCOL_780_854:
		return Protocol4::FLUID;
	case ProtocolVersion::PROTOCOL_860_986:
		return Protocol5::FLUID;
	case ProtocolVersion::PROTOCOL_1010_PLUS:
		return Protocol6::FLUID;
	default:
		return Protocol5::FLUID;
	}
}

} // namespace MetadataFlags
