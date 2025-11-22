#include "PreviewManager.h"
#include "TextureManager.h"
#include "ThingTypeHelper.h"
#include "ConfigManager.h"
#include "../Misc/Warninger.h"
#include "../Misc/definitions.h"
#include "../Things/Items.h"
#include "../Things/Outfits.h"
#include "../Things/Effects.h"
#include "../Things/Missiles.h"

PreviewManager::PreviewManager(TextureManager* textureManager) 
    : textureManager(textureManager) {
}

std::vector<std::shared_ptr<sf::Texture>>& PreviewManager::getPreviewTexturesVector(ThingCategory category) {
    switch(category) {
        case ThingCategory::ITEM:
            return previewTexturesItems;
        case ThingCategory::OUTFIT:
            return previewTexturesOutfits;
        case ThingCategory::EFFECT:
            return previewTexturesEffects;
        case ThingCategory::MISSILE:
            return previewTexturesMissiles;
        default:
            return previewTexturesItems;
    }
}

std::shared_ptr<ThingType> PreviewManager::getThingType(int id, ThingCategory category, bool& isValid) {
    isValid = false;
    std::shared_ptr<ThingType> thingType;
    
    switch(category) {
        case ThingCategory::ITEM:
            isValid = Items::isValidItemTypeIndex(id);
            if (isValid) thingType = Items::getItemType(id);
            break;
        case ThingCategory::OUTFIT:
            isValid = Outfits::isValidOutfitTypeIndex(id);
            if (isValid) thingType = Outfits::getOutfitType(id);
            break;
        case ThingCategory::EFFECT:
            isValid = Effects::isValidEffectTypeIndex(id);
            if (isValid) thingType = Effects::getEffectType(id);
            break;
        case ThingCategory::MISSILE:
            isValid = Missiles::isValidMissileTypeIndex(id);
            if (isValid) thingType = Missiles::getMissileType(id);
            break;
    }
    
    return thingType;
}

std::shared_ptr<sf::Texture> PreviewManager::getPreviewTexture(int thingTypeId, ThingCategory category) {
    auto& previewTextures = getPreviewTexturesVector(category);
    if(thingTypeId < previewTextures.size() && previewTextures.at(thingTypeId)) {
        return previewTextures[thingTypeId];
    }
    return textureManager->getBlankTexture();
}

void PreviewManager::replacePreviewTexture(int thingTypeId, std::shared_ptr<sf::Texture> texture, ThingCategory category) {
    auto& previewTextures = getPreviewTexturesVector(category);
    if(thingTypeId < 0) {
        return;
    }
    if(thingTypeId >= previewTextures.size()) {
        previewTextures.resize(thingTypeId + 1);
    }
    previewTextures[thingTypeId] = texture;
}

void PreviewManager::setDecoyPreviewTexture(int id, ThingCategory category) {
    replacePreviewTexture(id, std::make_shared<sf::Texture>(), category);
}

void PreviewManager::clearPreviewTextures() {
    previewTexturesItems.clear();
    previewTexturesOutfits.clear();
    previewTexturesEffects.clear();
    previewTexturesMissiles.clear();
    previewTexturesItems.shrink_to_fit();
    previewTexturesOutfits.shrink_to_fit();
    previewTexturesEffects.shrink_to_fit();
    previewTexturesMissiles.shrink_to_fit();
}

sf::Texture PreviewManager::getThingSpriteSheet(int thingTypeId, int animations, ThingCategory category) {
    bool isValid = false;
    auto thingType = getThingType(thingTypeId, category, isValid);
    
    if (!isValid || !thingType) {
        Warninger::sendWarning(FUNC_NAME, "Invalid thingType id: " + std::to_string(thingTypeId));
        return sf::Texture(*textureManager->getBlankTexture());
    }

    if (thingType->width == 0 || thingType->height == 0 || thingType->width > 255 || thingType->height > 255) {
        Warninger::sendWarning(FUNC_NAME, "Invalid dimensions for ThingType (" + std::to_string(thingTypeId) + 
                               "): width=" + std::to_string(thingType->width) + ", height=" + std::to_string(thingType->height));
        return sf::Texture(*textureManager->getBlankTexture());
    }

    if (animations <= 0) {
        animations = 1;
    }

    auto spriteMaxSize = ConfigManager::getInstance()->getSpriteMaxSize();
    int singleAnimationFrameSize = thingType->height * spriteMaxSize;

    sf::Vector2u size{
            static_cast<unsigned>(thingType->width * spriteMaxSize),
            static_cast<unsigned>(singleAnimationFrameSize * animations)
    };
    
    if (size.x == 0 || size.y == 0 || size.x > 8192 || size.y > 8192) {
        Warninger::sendWarning(FUNC_NAME, "Invalid texture size for ThingType (" + std::to_string(thingTypeId) + 
                               "): " + std::to_string(size.x) + "x" + std::to_string(size.y));
        return sf::Texture(*textureManager->getBlankTexture());
    }
    
    sf::RenderTexture render(size);
    if (render.getSize().x == 0 || render.getSize().y == 0) {
        Warninger::sendWarning(FUNC_NAME, "Failed to create RenderTexture for ThingType (" + std::to_string(thingTypeId) + ")");
        return sf::Texture(*textureManager->getBlankTexture());
    }

    render.clear(sf::Color::Transparent);

    for (int a = 1; a <= animations; a++) {
        for (int l = 0; l < thingType->layers; l++) {
            for (int w = 0; w < thingType->width; w++) {
                for (int h = 0; h < thingType->height; h++) {
                    uint32_t textureId = ThingTypeHelper::getTextureIdFromThingType(thingType, w, h, a, l, 0, 0, 0);
                    auto texture = textureManager->getTexture(textureId);
                    if (texture) {
                        sf::Sprite sprite(*texture);
                        sprite.setPosition(sf::Vector2f(
                                static_cast<float>((thingType->width - w - 1) * spriteMaxSize),
                                static_cast<float>((thingType->height - h - 1) * spriteMaxSize + ((a - 1) * singleAnimationFrameSize))
                        ));
                        render.draw(sprite);
                    }
                }
            }
        }
    }
    render.display();

    sf::Texture spriteSheetTexture = sf::Texture(render.getTexture());
    return spriteSheetTexture;
}

void PreviewManager::createPreviewTexture(int id, ThingCategory category) {
    bool isValid = false;
    auto thingType = getThingType(id, category, isValid);
    
    auto& previewTextures = getPreviewTexturesVector(category);
    
    if (!isValid || !thingType || thingType->width == 0 || thingType->height == 0) {
        if (id >= previewTextures.size()) {
            previewTextures.resize(id + 1);
        }
        replacePreviewTexture(id, textureManager->getBlankTexture(), category);
        return;
    }

    auto thingPreviewTexture = getThingSpriteSheet(id, 1, category);
    
    if (thingPreviewTexture.getSize().x == 0 || thingPreviewTexture.getSize().y == 0) {
        if (id >= previewTextures.size()) {
            previewTextures.resize(id + 1);
        }
        replacePreviewTexture(id, textureManager->getBlankTexture(), category);
        return;
    }

    auto finalSprite = sf::Sprite(thingPreviewTexture);
    auto SizeOfButtonSprite = ConfigManager::getInstance()->getSpriteButtonSize();
    finalSprite.setScale({
         static_cast<float>(SizeOfButtonSprite.x) / thingPreviewTexture.getSize().x,
         static_cast<float>(SizeOfButtonSprite.y) / thingPreviewTexture.getSize().y
     });

    sf::RenderTexture scaledRender (sf::Vector2u(SizeOfButtonSprite.x, SizeOfButtonSprite.y));
    if (scaledRender.getSize().x == 0 || scaledRender.getSize().y == 0) {
        if (id >= previewTextures.size()) {
            previewTextures.resize(id + 1);
        }
        replacePreviewTexture(id, textureManager->getBlankTexture(), category);
        return;
    }
    
    scaledRender.clear(sf::Color::Transparent);
    scaledRender.draw(finalSprite);
    scaledRender.display();

    auto texture = std::make_shared<sf::Texture>(scaledRender.getTexture());
    if (id >= previewTextures.size()) {
        previewTextures.resize(id + 1);
    }
    replacePreviewTexture(id, texture, category);
}

void PreviewManager::createPreviewTexturesForPage(int pageFirstThingType, int pageLastThingType, ThingCategory category) {
    for (int id = pageFirstThingType; id <= pageLastThingType; ++id) {
        bool isValid = false;
        auto thingType = getThingType(id, category, isValid);
        
        auto& previewTextures = getPreviewTexturesVector(category);
        
        if (!isValid || !thingType || thingType->width == 0 || thingType->height == 0) {
            if (id >= previewTextures.size()) {
                previewTextures.resize(id + 1);
            }
            replacePreviewTexture(id, textureManager->getBlankTexture(), category);
            continue;
        }
        
        try {
            createPreviewTexture(id, category);
        } catch (const std::exception& e) {
            Warninger::sendWarning(FUNC_NAME, "Exception creating preview for thing " + std::to_string(id) + ": " + e.what());
            if (id >= previewTextures.size()) {
                previewTextures.resize(id + 1);
            }
            replacePreviewTexture(id, textureManager->getBlankTexture(), category);
        }
    }
}

