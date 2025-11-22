#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include "../Things/ThingType.h"
#include "../Things/ThingCategory.h"

// Forward declarations
class TextureManager;
class ThingType;

/**
 * @brief Manages preview texture generation and caching
 * 
 * This class is responsible for:
 * - Generating preview textures for ThingTypes
 * - Caching preview textures per category
 * - Creating sprite sheets for ThingTypes
 */
class PreviewManager {
public:
    explicit PreviewManager(TextureManager* textureManager);
    ~PreviewManager() = default;

    // Preview texture access
    std::shared_ptr<sf::Texture> getPreviewTexture(int thingTypeId, ThingCategory category = ThingCategory::ITEM);
    void replacePreviewTexture(int thingTypeId, std::shared_ptr<sf::Texture> texture, ThingCategory category = ThingCategory::ITEM);
    void setDecoyPreviewTexture(int id, ThingCategory category = ThingCategory::ITEM);

    // Preview texture generation
    /**
     * @brief Creates preview texture for ThingType
     *
     * This method is very costly, since it copies, makes textures
     * and makes render texture. It should be used with caution.
     *
     * @param id ThingType id
     * @param category Category of the thing type
     */
    void createPreviewTexture(int id, ThingCategory category = ThingCategory::ITEM);
    
    /**
     * @brief Creates preview texture for all things on page
     *
     * This method is very costly, since it copies, makes textures
     * and makes render texture. It should be used with caution.
     *
     * @param pageFirstThingType Id of first thingType on page
     * @param pageLastThingType Id of last thingType on page
     * @param category Category of the thing types
     */
    void createPreviewTexturesForPage(int pageFirstThingType, int pageLastThingType, ThingCategory category = ThingCategory::ITEM);
    void clearPreviewTextures();

    /**
     * @brief Returns a sprite sheet of a thing type
     *
     * Example use, is to call this method with animations = 1, which
     * will effectively help us with creating a preview texture for a thingtype.
     * Another example, is animations = max thing's animations, to get full sprite sheet
     * used in exporting the thing's png etc.
     *
     * @param thingTypeId id of thingType that we will be creating sprite sheet based on
     * @param animations of how many thing's animation frames should the sprite sheet be composed of
     * @param category Category of the thing type
     * @return sf::Texture that is composed of however many animation frames were requested in animations param
     */
    sf::Texture getThingSpriteSheet(int thingTypeId, int animations, ThingCategory category = ThingCategory::ITEM);

private:
    TextureManager* textureManager;

    // Separate preview texture storage for each category
    std::vector<std::shared_ptr<sf::Texture>> previewTexturesItems;
    std::vector<std::shared_ptr<sf::Texture>> previewTexturesOutfits;
    std::vector<std::shared_ptr<sf::Texture>> previewTexturesEffects;
    std::vector<std::shared_ptr<sf::Texture>> previewTexturesMissiles;
    
    // Helper to get the correct preview texture vector for a category
    std::vector<std::shared_ptr<sf::Texture>>& getPreviewTexturesVector(ThingCategory category);
    
    // Helper to get ThingType from category and ID
    std::shared_ptr<ThingType> getThingType(int id, ThingCategory category, bool& isValid);
};

