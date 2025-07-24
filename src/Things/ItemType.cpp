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
    return width * height * animationsFrames;
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