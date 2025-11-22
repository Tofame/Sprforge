# Refactoring Plan: Decoupling AssetsManager and ScrollableWindows

## Current Problems

### 1. **AssetsManager is a God Object**
AssetsManager currently handles:
- ✅ Texture storage and management
- ✅ Preview texture management (for multiple categories)
- ✅ File I/O (loading/saving .spr and .dat files)
- ✅ UI state management (lastSelectedItemId, lastSelectedCategory, animationFrameSetting)
- ✅ Unsaved changes tracking
- ✅ UI rendering (drawAssetsManagerControls)
- ✅ Compilation logic
- ✅ Export functionality

### 2. **Tight Coupling**
- ScrollableWindows directly depend on AssetsManager pointer
- Windows call many methods on AssetsManager (155+ references found)
- Shared responsibilities between classes
- No clear separation of concerns

### 3. **Shared Responsibilities**
- Both AssetsManager and ScrollableWindows manage selection state
- Both handle unsaved changes logic
- UI concerns mixed with data management

## Proposed Solution: Layered Architecture

### Phase 1: Extract UI State Management

**Create `UIStateManager` class:**
```cpp
class UIStateManager {
    // Manages UI-specific state that doesn't belong in data layer
    - lastSelectedItemId
    - lastSelectedCategory
    - animationFrameSetting
    - unsavedChanges tracking per category
    - unsavedItemType management
};
```

**Benefits:**
- Removes UI state from AssetsManager
- Single source of truth for UI state
- Easier to test and maintain

### Phase 2: Extract Texture Management

**Create `TextureManager` class:**
```cpp
class TextureManager {
    // Manages texture storage and retrieval
    - textures vector
    - getTexture(id)
    - replaceTexture(id, texture)
    - removeTexture(id)
    - isValidTextureIndex(id)
    - BLANK_TEXTURE
};
```

**Benefits:**
- Clear responsibility: texture storage
- AssetsManager becomes a coordinator, not a storage
- Easier to add features like texture caching, lazy loading

### Phase 3: Extract Preview Management

**Create `PreviewManager` class:**
```cpp
class PreviewManager {
    // Manages preview texture generation and caching
    - previewTextures per category
    - createPreviewTexture(id, category)
    - getPreviewTexture(id, category)
    - clearPreviewTextures()
    - getThingSpriteSheet(id, animations, category)
};
```

**Benefits:**
- Separates preview generation from core texture management
- Can optimize preview generation independently
- Clearer code organization

### Phase 4: Extract File I/O

**Create separate file handlers:**
```cpp
class SprFileHandler {
    - loadSpr(filePath) -> TextureManager
    - compileSpr(TextureManager, outputPath)
};

class DatFileHandler {
    - loadDat(filePath) -> Items/Outfits/etc.
    - compileDat(Items/Outfits/etc., outputPath)
};
```

**Benefits:**
- File I/O separated from business logic
- Easier to test file operations
- Can swap file format implementations

### Phase 5: Refactor AssetsManager

**New AssetsManager becomes a coordinator:**
```cpp
class AssetsManager {
    // Coordinates between different managers
    - TextureManager* textureManager
    - PreviewManager* previewManager
    - UIStateManager* uiStateManager
    - SprFileHandler* sprHandler
    - DatFileHandler* datHandler
    
    // High-level operations that coordinate multiple managers
    - loadAssets(sprPath, datPath)
    - compileAssets(outputPath)
    - isCompilable()
};
```

**Benefits:**
- AssetsManager becomes a facade/coordinator
- Clear separation of concerns
- Each component can be tested independently

### Phase 6: Refactor ScrollableWindows

**Use dependency injection:**
```cpp
class ItemsScrollableWindow {
    // Only depend on what you need
    - TextureManager* textureManager (for texture access)
    - PreviewManager* previewManager (for previews)
    - UIStateManager* uiStateManager (for UI state)
    - Items* items (for item data)
    
    // No direct dependency on AssetsManager!
};
```

**Benefits:**
- Windows only depend on what they actually use
- Easier to test (can mock dependencies)
- Clearer dependencies

## Implementation Strategy

### Step 1: Create UIStateManager (Low Risk)
- Extract UI state from AssetsManager
- Update ScrollableWindows to use UIStateManager
- Keep AssetsManager methods as wrappers for backward compatibility

### Step 2: Create TextureManager (Medium Risk)
- Extract texture storage from AssetsManager
- Update all texture access to go through TextureManager
- AssetsManager delegates to TextureManager

### Step 3: Create PreviewManager (Medium Risk)
- Extract preview texture logic
- Update preview generation calls
- AssetsManager coordinates between TextureManager and PreviewManager

### Step 4: Extract File I/O (Higher Risk)
- Create file handler classes
- Move file I/O logic
- Update load/compile methods

### Step 5: Refactor AssetsManager (Higher Risk)
- Make AssetsManager a coordinator
- Remove direct data storage
- Update all callers

### Step 6: Refactor ScrollableWindows (Medium Risk)
- Update constructors to take specific dependencies
- Remove AssetsManager dependency
- Update main.cpp

## Migration Path

1. **Add new classes alongside existing code** (no breaking changes)
2. **Make AssetsManager delegate to new classes** (backward compatible)
3. **Gradually update ScrollableWindows** to use new classes
4. **Remove old code** once everything is migrated

## Benefits After Refactoring

1. **Clear Responsibilities**: Each class has one clear purpose
2. **Reduced Coupling**: Classes depend on interfaces, not concrete implementations
3. **Easier Testing**: Can test each component independently
4. **Better Maintainability**: Changes to one component don't affect others
5. **Easier to Extend**: Can add new features without touching existing code

## Example: Before vs After

### Before:
```cpp
// ItemsScrollableWindow depends on everything
ItemsScrollableWindow(sf::RenderWindow& window, AssetsManager* am) {
    assetsManager = am;  // Has access to everything
}

void selectItem(int id) {
    assetsManager->setLastSelectedCategory(CATEGORY_ITEMS);
    assetsManager->setAnimationFrameSetting(1);
    assetsManager->setLastSelectedItemId(id);
    assetsManager->hasUnsavedChanges(CATEGORY_ITEMS_ITEMTYPE);
    assetsManager->createPreviewTexturesForPage(...);
}
```

### After:
```cpp
// ItemsScrollableWindow depends only on what it needs
ItemsScrollableWindow(
    sf::RenderWindow& window,
    TextureManager* textureManager,
    PreviewManager* previewManager,
    UIStateManager* uiState,
    Items* items
) {
    this->textureManager = textureManager;
    this->previewManager = previewManager;
    this->uiState = uiState;
    this->items = items;
}

void selectItem(int id) {
    uiState->setLastSelectedCategory(CATEGORY_ITEMS);
    uiState->setAnimationFrameSetting(1);
    uiState->setLastSelectedItemId(id);
    if (uiState->hasUnsavedChanges(CATEGORY_ITEMS_ITEMTYPE)) {
        // handle unsaved changes
    }
    previewManager->createPreviewTexturesForPage(...);
}
```

## Notes

- This is a large refactoring, should be done incrementally
- Keep backward compatibility during migration
- Test thoroughly after each phase
- Consider using interfaces/abstract classes for better testability

