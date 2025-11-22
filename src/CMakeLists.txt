add_executable(Sprforge main.cpp
        SpritesScrollableWindow.cpp
        SpritesScrollableWindow.h
        ResourceManagers/AssetsManager.cpp
        ResourceManagers/AssetsManager.h
        Misc/tools.h
        Things/Thing.cpp
        Things/Thing.h
        Things/ThingType.cpp
        Things/ThingType.h
        Things/ThingCategory.h
        Things/Item.cpp
        Things/Item.h
        Things/ItemType.cpp
        Things/ItemType.h
        Things/Items.cpp
        Things/Items.h
        Things/OutfitType.cpp
        Things/OutfitType.h
        Things/Outfits.cpp
        Things/Outfits.h
        Things/EffectType.cpp
        Things/EffectType.h
        Things/Effects.cpp
        Things/Effects.h
        Things/MissileType.cpp
        Things/MissileType.h
        Things/Missiles.cpp
        Things/Missiles.h
        ItemsScrollableWindow.h
        ItemsScrollableWindow.cpp
        OutfitsScrollableWindow.h
        OutfitsScrollableWindow.cpp
        EffectsScrollableWindow.h
        EffectsScrollableWindow.cpp
        MissilesScrollableWindow.h
        MissilesScrollableWindow.cpp
        ResourceManagers/ConfigManager.cpp
        ResourceManagers/ConfigManager.h
        Misc/Warninger.h
        Helper/GUIHelper.cpp
        Helper/GUIHelper.h
        Helper/SavedData.cpp
        Helper/SavedData.h
        Misc/definitions.h
        Helper/DropManager.h
        Misc/Timer.h
)

target_link_libraries(Sprforge PRIVATE ImGui-SFML::ImGui-SFML nfd fmt)
target_include_directories(Sprforge PRIVATE ${tomlplusplus_SOURCE_DIR}/include)

# macOS-specific configuration
if(APPLE)
    # Link macOS frameworks needed for clipboard and drag-and-drop
    target_link_libraries(Sprforge PRIVATE 
        "-framework AppKit"
        "-framework CoreGraphics"
        "-framework Foundation"
    )
    
    # Set Objective-C++ standard for files that include Objective-C++ code
    set_target_properties(Sprforge PROPERTIES
        OBJCXX_STANDARD 17
        OBJCXX_STANDARD_REQUIRED ON
    )
    
    # Enable Objective-C++ for source files that include tools.h
    # Since tools.h contains Objective-C++ code, we need to compile C++ files as Objective-C++
    set_source_files_properties(
        main.cpp
        SpritesScrollableWindow.cpp
        ItemsScrollableWindow.cpp
        OutfitsScrollableWindow.cpp
        EffectsScrollableWindow.cpp
        MissilesScrollableWindow.cpp
        ResourceManagers/AssetsManager.cpp
        ResourceManagers/ConfigManager.cpp
        Helper/GUIHelper.cpp
        Helper/SavedData.cpp
        Things/Item.cpp
        Things/Thing.cpp
        Things/ThingType.cpp
        Things/ItemType.cpp
        Things/Items.cpp
        PROPERTIES COMPILE_FLAGS "-x objective-c++"
    )
endif()

# Copy DLLs needed for runtime on Windows
if(WIN32)
  if (BUILD_SHARED_LIBS)
    add_custom_command(TARGET Sprforge POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:ImGui-SFML::ImGui-SFML>
          $<TARGET_FILE:sfml-graphics>
          $<TARGET_FILE:sfml-window>
          $<TARGET_FILE:sfml-system>
          $<TARGET_FILE:nfd>
          $<TARGET_FILE:fmt>
          $<TARGET_FILE_DIR:Sprforge>)
  endif()
endif()