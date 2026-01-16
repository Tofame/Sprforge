#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace sf {

template <typename T>
struct Vector2 {
	T x;
	T y;

	constexpr Vector2() : x(0), y(0) {}
	constexpr Vector2(T xVal, T yVal) : x(xVal), y(yVal) {}
};

using Vector2u = Vector2<unsigned int>;
using Vector2i = Vector2<int>;
using Vector2f = Vector2<float>;

struct Color {
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	uint8_t a = 255;

	constexpr Color() = default;
	constexpr Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
		: r(red), g(green), b(blue), a(alpha) {}

	static const Color Transparent;
};

class Image {
public:
	Image();
	Image(Vector2u size, Color fillColor);
	Image(Vector2u size, const uint8_t* pixels);

	bool loadFromFile(const std::string& path);
	bool saveToFile(const std::string& path) const;

	Vector2u getSize() const { return size_; }

	void setPixel(Vector2u position, Color color);
	Color getPixel(Vector2u position) const;

	const uint8_t* getPixelsPtr() const { return pixels_.empty() ? nullptr : pixels_.data(); }
	uint8_t* getPixelsPtr() { return pixels_.empty() ? nullptr : pixels_.data(); }

	const std::vector<uint8_t>& rawPixels() const { return pixels_; }
	std::vector<uint8_t>& rawPixels() { return pixels_; }

private:
	Vector2u size_{0, 0};
	std::vector<uint8_t> pixels_;
};

class Texture {
public:
	Texture();
	Texture(const Texture& other);
	Texture(Texture&& other) noexcept;
	Texture& operator=(const Texture& other);
	Texture& operator=(Texture&& other) noexcept;
	~Texture();

	bool loadFromFile(const std::string& path);
	bool loadFromImage(const Image& image);

	Image copyToImage() const;
	Vector2u getSize() const { return size_; }
	unsigned int getNativeHandle() const { return glHandle_; }

private:
	void destroy();
	bool upload();

	Vector2u size_{0, 0};
	std::vector<uint8_t> pixels_;
	unsigned int glHandle_ = 0;
};

class Time {
public:
	Time();
	explicit Time(double seconds);

	float asSeconds() const { return static_cast<float>(seconds_); }

private:
	double seconds_;
};

class Clock {
public:
	Clock();

	Time restart();
	Time getElapsedTime() const;

private:
	std::chrono::steady_clock::time_point start_;
};

} // namespace sf
