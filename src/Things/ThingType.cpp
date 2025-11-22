#include "ThingType.h"

ThingType::ThingType() {
    textureIdsVector.reserve(6);
    textureIdsVector.push_back(0);
}

void ThingType::setWidth(int _width) {
    if (_width < 1) _width = 1;
    width = static_cast<uint8_t>(_width);
    updateTextureVectorSize();
}

void ThingType::setHeight(int _height) {
    if (_height < 1) _height = 1;
    height = static_cast<uint8_t>(_height);
    updateTextureVectorSize();
}

void ThingType::setAnimationCount(int count) {
    if (count < 1) count = 1;
    animationsFrames = static_cast<uint8_t>(count);
    updateTextureVectorSize();
}

void ThingType::setLayers(int _layers) {
    if (_layers < 1) _layers = 1;
    layers = static_cast<uint8_t>(_layers);
    updateTextureVectorSize();
}

void ThingType::setPatternX(int _patternX) {
    if (_patternX < 1) _patternX = 1;
    patternX = static_cast<uint8_t>(_patternX);
    updateTextureVectorSize();
}

void ThingType::setPatternY(int _patternY) {
    if (_patternY < 1) _patternY = 1;
    patternY = static_cast<uint8_t>(_patternY);
    updateTextureVectorSize();
}

void ThingType::setPatternZ(int _patternZ) {
    if (_patternZ < 1) _patternZ = 1;
    patternZ = static_cast<uint8_t>(_patternZ);
    updateTextureVectorSize();
}

int ThingType::getCalcIndexesCount() const {
    return width * height * layers * patternX * patternY * patternZ * animationsFrames;
}

void ThingType::updateTextureVectorSize() {
    int total = getCalcIndexesCount();
    textureIdsVector.resize(total, 0);
}

bool ThingType::operator==(const ThingType& other) const {
    return category == other.category &&
           width == other.width &&
           height == other.height &&
           animationsFrames == other.animationsFrames &&
           patternX == other.patternX &&
           patternY == other.patternY &&
           patternZ == other.patternZ &&
           layers == other.layers &&
           textureIdsVector == other.textureIdsVector;
}

bool ThingType::operator!=(const ThingType& other) const {
    return !(*this == other);
}

