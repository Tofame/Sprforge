#pragma once

#include "Graphics/SFMLCompat.h"
#include "Misc/definitions.h"
#include "Misc/tools.h"
#include "ResourceManagers/AssetsManager.h"
#include "Things/ThingCategory.h"
#include <imgui.h>

/**
 * @brief Base class for scrollable windows displaying thing types (Items, Outfits, Effects, Missiles)
 *
 * This class provides common functionality for pagination, selection, and UI controls.
 * Derived classes must implement type-specific methods.
 */
class ThingScrollableWindow {
public:
	ThingScrollableWindow(AssetsManager* am, ThingCategory category);
	virtual ~ThingScrollableWindow() = default;

	// Pure virtual methods - must be implemented by derived classes
	virtual void drawTypeList(sf::Clock& deltaClock) = 0;
	virtual void drawTypePanel() = 0;
	virtual void selectType(int id, bool goToSelect = true) = 0;
	virtual int addType() = 0;
	virtual bool removeType() = 0;

	// Virtual methods with default implementations
	virtual void drawPaginationControls();

	// Common getters - virtual so derived classes can override if needed
	virtual int getTotalButtons() const = 0;
	virtual int getSelectedButtonIndex() = 0;
	virtual bool isAnyButtonSelected() = 0;

	// Pagination methods
	int getCurrentPage() const { return currentPage; }
	void setCurrentPage(int page) { currentPage = page; }
	int getPageFirstIndex() const { return getCurrentPage() * ConfigManager::getInstance()->getButtonsCountItemPage(); }
	int getPageLastIndex() const {
		return std::min(getPageFirstIndex() + ConfigManager::getInstance()->getButtonsCountItemPage(),
						getTotalButtons());
	}

	void incrementPage();
	void decrementPage();

	// Virtual hook for page change validation (Items uses this for unsaved changes)
	virtual bool onPageChange() { return true; }

	// Virtual hook for page changed callback
	virtual void onPageChanged(int oldPage, int newPage, bool autoSelectFirst = true);

protected:
	AssetsManager* assetsManager;
	ThingCategory category;

	int currentPage = 0;
	int scrollToButtonIndex = -1;
	char idInputBuffer[10];
	bool drawGrid = true;

	// Animation playback
	bool isAnimationPlaying = false;
	sf::Clock animationClock;

	// Helper methods for pagination controls
	void drawPaginationRow(int startIndex, int endIndex, const char* typeName, const char* inputLabel);
	void drawActionButtonsRow(const char* newButtonLabel, const char* removeButtonLabel);

	// Helper to get button IDs for pagination
	std::string getPageDecButtonId() const;
	std::string getPageIncButtonId() const;
	std::string getInputTextFieldId() const;
	std::string getNewButtonId() const;
	std::string getRemoveButtonId() const;

	// Helper methods for drawing needs shared between child class windows
	void drawLightControlSegment(ThingType& thing);
};
