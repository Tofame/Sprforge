#include "SFMLCompat.h"

#include <cctype>
#include <filesystem>
#include <stdexcept>

#include <GLES2/gl2.h>
#include <EGL/egl.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace sf {

const Color Color::Transparent = Color(0, 0, 0, 0);

namespace {

inline size_t pixelIndex(Vector2u size, Vector2u position) {
    return static_cast<size_t>(position.y * size.x + position.x) * 4;
}

} // namespace

Image::Image() : size_{0, 0} {}

Image::Image(Vector2u size, Color fillColor) : size_(size) {
    pixels_.assign(static_cast<size_t>(size.x) * size.y * 4, 0);
    for (size_t i = 0; i < pixels_.size(); i += 4) {
        pixels_[i + 0] = fillColor.r;
        pixels_[i + 1] = fillColor.g;
        pixels_[i + 2] = fillColor.b;
        pixels_[i + 3] = fillColor.a;
    }
}

Image::Image(Vector2u size, const uint8_t* pixels) : size_(size) {
    if (size.x == 0 || size.y == 0) {
        return;
    }

    size_t count = static_cast<size_t>(size.x) * size.y * 4;
    pixels_.assign(pixels, pixels + count);
}

bool Image::loadFromFile(const std::string& path) {
    int width = 0;
    int height = 0;
    int channels = 0;

    stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data) {
        return false;
    }

    size_ = Vector2u(static_cast<unsigned>(width), static_cast<unsigned>(height));
    pixels_.assign(data, data + (static_cast<size_t>(width) * height * 4));

    stbi_image_free(data);
    return true;
}

bool Image::saveToFile(const std::string& path) const {
    if (pixels_.empty() || size_.x == 0 || size_.y == 0) {
        return false;
    }

    auto extension = std::filesystem::path(path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    const int stride = static_cast<int>(size_.x * 4);
    if (extension == ".png") {
        return stbi_write_png(path.c_str(), static_cast<int>(size_.x), static_cast<int>(size_.y), 4, pixels_.data(), stride) != 0;
    }
    if (extension == ".bmp") {
        return stbi_write_bmp(path.c_str(), static_cast<int>(size_.x), static_cast<int>(size_.y), 4, pixels_.data()) != 0;
    }
    if (extension == ".jpg" || extension == ".jpeg") {
        // stbi_write_jpg expects RGB, but we have RGBA. Convert on the fly.
        std::vector<uint8_t> rgb(static_cast<size_t>(size_.x) * size_.y * 3);
        for (size_t i = 0, j = 0; i < pixels_.size(); i += 4, j += 3) {
            rgb[j + 0] = pixels_[i + 0];
            rgb[j + 1] = pixels_[i + 1];
            rgb[j + 2] = pixels_[i + 2];
        }
        return stbi_write_jpg(path.c_str(), static_cast<int>(size_.x), static_cast<int>(size_.y), 3, rgb.data(), 90) != 0;
    }

    // Default to PNG if no extension or unsupported.
    return stbi_write_png(path.c_str(), static_cast<int>(size_.x), static_cast<int>(size_.y), 4, pixels_.data(), stride) != 0;
}

void Image::setPixel(Vector2u position, Color color) {
    if (position.x >= size_.x || position.y >= size_.y) {
        return;
    }

    size_t index = pixelIndex(size_, position);
    pixels_[index + 0] = color.r;
    pixels_[index + 1] = color.g;
    pixels_[index + 2] = color.b;
    pixels_[index + 3] = color.a;
}

Color Image::getPixel(Vector2u position) const {
    if (position.x >= size_.x || position.y >= size_.y || pixels_.empty()) {
        return Color();
    }

    size_t index = pixelIndex(size_, position);
    return Color(pixels_[index + 0], pixels_[index + 1], pixels_[index + 2], pixels_[index + 3]);
}

Texture::Texture() = default;

Texture::Texture(const Texture& other)
    : size_(other.size_), pixels_(other.pixels_) {
    upload();
}

Texture::Texture(Texture&& other) noexcept
    : size_(other.size_), pixels_(std::move(other.pixels_)), glHandle_(other.glHandle_) {
    other.glHandle_ = 0;
    other.size_ = Vector2u{0, 0};
}

Texture& Texture::operator=(const Texture& other) {
    if (this == &other) {
        return *this;
    }
    size_ = other.size_;
    pixels_ = other.pixels_;
    upload();
    return *this;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    destroy();
    size_ = other.size_;
    pixels_ = std::move(other.pixels_);
    glHandle_ = other.glHandle_;
    other.glHandle_ = 0;
    other.size_ = Vector2u{0, 0};
    return *this;
}

Texture::~Texture() {
    destroy();
}

bool Texture::loadFromFile(const std::string& path) {
    Image image;
    if (!image.loadFromFile(path)) {
        return false;
    }
    return loadFromImage(image);
}

bool Texture::loadFromImage(const Image& image) {
    size_ = image.getSize();
    pixels_ = image.rawPixels();
    return upload();
}

Image Texture::copyToImage() const {
    return Image(size_, pixels_.data());
}

void Texture::destroy() {
    if (glHandle_ != 0) {
        glDeleteTextures(1, &glHandle_);
        glHandle_ = 0;
    }
}

bool Texture::upload() {
    if (size_.x == 0 || size_.y == 0 || pixels_.empty()) {
        destroy();
        return false;
    }

    if (glHandle_ == 0) {
        glGenTextures(1, &glHandle_);
    }

    glBindTexture(GL_TEXTURE_2D, glHandle_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        static_cast<GLsizei>(size_.x),
        static_cast<GLsizei>(size_.y),
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels_.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

Time::Time() : seconds_(0.0) {}

Time::Time(double seconds) : seconds_(seconds) {}

Clock::Clock() : start_(std::chrono::steady_clock::now()) {}

Time Clock::restart() {
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = now - start_;
    start_ = now;
    return Time(diff.count());
}

Time Clock::getElapsedTime() const {
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = now - start_;
    return Time(diff.count());
}

} // namespace sf

