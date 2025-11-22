#include "ItemType.h"

ItemType::ItemType() {
    textureIdsVector.reserve(6);
    textureIdsVector.push_back(0);
}

void ItemType::setItemTypeWidth(int _width) {
    int oldWidth = this->width;
    this->width = _width;

    onItemTypeWidthChanged(oldWidth, _width);
}

void ItemType::onItemTypeWidthChanged(int oldWith, int newWidth) {
    if(oldWith == newWidth) {
        return;
    }

    int total = getCalcIndexesCount();
    this->textureIdsVector.resize(total, 0);
}

int ItemType::getCalcIndexesCount() const {
    return width * height * layers * patternX * patternY * patternZ * animationsFrames;
}

void ItemType::setItemTypeHeight(int _height) {
    int oldHeight = this->height;
    this->height = _height;

    onItemTypeHeightChanged(oldHeight, _height);
}

void ItemType::onItemTypeHeightChanged(int oldHeight, int height) {
    if(oldHeight == height) {
        return;
    }

    int total = getCalcIndexesCount();
    this->textureIdsVector.resize(total, 0);
}

void ItemType::setItemTypeAnimationCount(int count) {
    int oldAnimationFrames = this->animationsFrames;
    this->animationsFrames = count;

    onItemTypeAnimationFramesChanged(oldAnimationFrames, count);
}

void ItemType::onItemTypeAnimationFramesChanged(int oldAnimationCount, int animationCount) {
    if(oldAnimationCount == animationCount) {
        return;
    }

    int total = getCalcIndexesCount();
    this->textureIdsVector.resize(total, 0);
}

void ItemType::setItemTypeLayers(int _layers) {
    int oldLayers = this->layers;
    this->layers = _layers;
    
    if(oldLayers != _layers) {
        int total = getCalcIndexesCount();
        this->textureIdsVector.resize(total, 0);
    }
}

void ItemType::setItemTypePatternX(int _patternX) {
    int oldPatternX = this->patternX;
    this->patternX = _patternX;
    
    if(oldPatternX != _patternX) {
        int total = getCalcIndexesCount();
        this->textureIdsVector.resize(total, 0);
    }
}

void ItemType::setItemTypePatternY(int _patternY) {
    int oldPatternY = this->patternY;
    this->patternY = _patternY;
    
    if(oldPatternY != _patternY) {
        int total = getCalcIndexesCount();
        this->textureIdsVector.resize(total, 0);
    }
}

void ItemType::setItemTypePatternZ(int _patternZ) {
    int oldPatternZ = this->patternZ;
    this->patternZ = _patternZ;
    
    if(oldPatternZ != _patternZ) {
        int total = getCalcIndexesCount();
        this->textureIdsVector.resize(total, 0);
    }
}