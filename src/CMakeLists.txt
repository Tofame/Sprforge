add_executable(Sprforge main.cpp
        SpritesScrollableWindow.cpp
        SpritesScrollableWindow.h
        ResourceManagers/AssetsManager.cpp
        ResourceManagers/AssetsManager.h
        Misc/tools.h
        Things/Thing.cpp
        Things/Thing.h
        Things/Item.cpp
        Things/Item.h
        Things/ItemType.cpp
        Things/ItemType.h
        Things/Items.cpp
        Things/Items.h
        ItemsScrollableWindow.h
        ItemsScrollableWindow.cpp
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