include(FetchContent)

# SFML
FetchContent_Declare(
  sfml
  GIT_REPOSITORY https://github.com/SFML/SFML
  GIT_TAG 016bea9491ccafc3529019fe1d403885a8b3a6ae # 3.0.1
)
add_subdirectory(sfml)

# Dear ImGui
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui
  GIT_TAG 1dfbb100d6ca6e7d813102e979c07f66a56546bb # 1.91.1
)

FetchContent_MakeAvailable(imgui)

# ImGui-SFML
FetchContent_Declare(
  imgui-sfml
  GIT_REPOSITORY https://github.com/SFML/imgui-sfml
  GIT_TAG 5e8f8fb857c8f3cff1096bf3e4402c64e3bcf6af # 3.0
)
add_subdirectory(imgui-sfml)

add_subdirectory(nativefiledialog-extended)

# toml++
FetchContent_Declare(
        tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG v3.4.0  # Use the latest stable version
)
add_subdirectory(tomlplusplus)

# fmt
FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 11.1.4
)
add_subdirectory(fmt)