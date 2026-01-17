#pragma once

#include <cstdint>
#include <vector>

// Animation mode constants (from Object Builder's AnimationMode.as)
namespace AnimationMode {
constexpr uint8_t ASYNCHRONOUS = 0;
constexpr uint8_t SYNCHRONOUS = 1;
} // namespace AnimationMode

// Frame duration structure (min/max time for each animation frame)
// Used in versions 10.50+ (improvedAnimations)
struct FrameDuration {
	uint32_t minimum = 100; // Minimum duration in milliseconds
	uint32_t maximum = 100; // Maximum duration in milliseconds

	FrameDuration() = default;
	FrameDuration(uint32_t min, uint32_t max) : minimum(min), maximum(max) {}

	// Get the actual duration (random between min and max, or min if equal)
	[[nodiscard]] uint32_t getDuration() const {
		if (minimum == maximum) {
			return minimum;
		}
		// In a real implementation, return random between min and max
		return minimum + static_cast<uint32_t>((static_cast<double>(rand()) / RAND_MAX) * (maximum - minimum));
	}

	bool operator==(const FrameDuration& other) const { return minimum == other.minimum && maximum == other.maximum; }
};

// Frame group structure (from Object Builder's FrameGroup.as)
// Used in versions 10.57+ for outfits with multiple animation groups
// (e.g., idle animation, walking animation)
struct FrameGroup {
	uint8_t type = 0; // Frame group type (0 = idle, 1 = walking for outfits)

	// Dimensions
	uint8_t width = 1;
	uint8_t height = 1;
	uint8_t exactSize = 32; // Default sprite size

	// Pattern data
	uint8_t layers = 1;
	uint8_t patternX = 1;
	uint8_t patternY = 1;
	uint8_t patternZ = 1;
	uint8_t frames = 1; // Number of animation frames

	// Animation data
	bool isAnimation = false;
	uint8_t animationMode = AnimationMode::ASYNCHRONOUS;
	int32_t loopCount = 0; // -1 = infinite, 0 = ping-pong, >0 = loop count
	int8_t startFrame = 0; // Starting animation frame
	std::vector<FrameDuration> frameDurations;

	// Sprite indices
	std::vector<uint32_t> spriteIndex;

	FrameGroup() = default;

	// Get total number of sprites in this frame group
	[[nodiscard]] uint32_t getTotalSprites() const {
		return static_cast<uint32_t>(width) * height * patternX * patternY * patternZ * frames * layers;
	}

	// Calculate sprite index for given parameters
	[[nodiscard]] uint32_t getSpriteIndex(uint8_t w, uint8_t h, uint8_t layer, uint8_t pX, uint8_t pY, uint8_t pZ,
										  uint8_t frame) const {
		return ((((((frame % frames) * patternZ + pZ) * patternY + pY) * patternX + pX) * layers + layer) * height +
				h) *
				   width +
			   w;
	}

	// Get frame duration for a specific frame (or default if not available)
	[[nodiscard]] FrameDuration getFrameDuration(uint8_t frameIndex) const {
		if (frameIndex < frameDurations.size()) {
			return frameDurations[frameIndex];
		}
		return FrameDuration(100, 100); // Default 100ms
	}

	// Clone this frame group
	[[nodiscard]] FrameGroup clone() const {
		FrameGroup copy;
		copy.type = type;
		copy.width = width;
		copy.height = height;
		copy.exactSize = exactSize;
		copy.layers = layers;
		copy.patternX = patternX;
		copy.patternY = patternY;
		copy.patternZ = patternZ;
		copy.frames = frames;
		copy.isAnimation = isAnimation;
		copy.animationMode = animationMode;
		copy.loopCount = loopCount;
		copy.startFrame = startFrame;
		copy.frameDurations = frameDurations;
		copy.spriteIndex = spriteIndex;
		return copy;
	}
};

// Default frame durations for different categories (from Object Builder)
namespace DefaultDurations {
constexpr uint32_t ITEM = 100;
constexpr uint32_t OUTFIT = 100;
constexpr uint32_t EFFECT = 100;
constexpr uint32_t MISSILE = 100;
} // namespace DefaultDurations
