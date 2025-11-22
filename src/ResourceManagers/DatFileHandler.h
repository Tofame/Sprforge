#pragma once

#include <string>
#include <cstdint>
#include <iosfwd>
#include <memory>

// Forward declarations
class ThingType;
struct AssetsInfo;

/**
 * @brief Handles .dat file loading and compilation
 * 
 * This class is responsible for:
 * - Loading .dat files (items, outfits, effects, missiles)
 * - Compiling ThingTypes into .dat files
 */
class DatFileHandler {
public:
    explicit DatFileHandler(AssetsInfo* assetsInfo);
    ~DatFileHandler() = default;

    /**
     * @brief Loads .dat file
     * 
     * @param datFilePath Path to the .dat file (empty = default path)
     * @return True if loading was successful
     */
    bool loadDat(const std::string& datFilePath);

    /**
     * @brief Compiles ThingTypes into a .dat file
     * 
     * @param outputFilePath Path to output .dat file
     * @param signature Signature to write to the file
     */
    void compileDat(const std::string& outputFilePath, uint32_t signature);

    /**
     * @brief Loads ThingType patterns from file stream
     * 
     * @param inFile Input file stream
     * @param thingType ThingType to load patterns into
     */
    void loadThingTypePatterns(std::istream& inFile, std::shared_ptr<ThingType> thingType);

    /**
     * @brief Writes ThingType patterns to file stream
     * 
     * @param outFile Output file stream
     * @param thingType ThingType to write patterns from
     */
    void writeThingTypePatterns(std::ostream& outFile, std::shared_ptr<ThingType> thingType);

private:
    AssetsInfo* assetsInfo; // Reference to assets info for format details
    
    // Helper methods for reading/writing ThingType data
    void readThingTypeDimensions(std::istream& inFile, std::shared_ptr<ThingType> thingType);
    void writeThingTypeDimensions(std::ostream& outFile, std::shared_ptr<ThingType> thingType);
};

