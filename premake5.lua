workspace "SprForge"
    configurations { "Debug", "Release" }
    architecture "x86_64"
    startproject "sprforge"

project "sprforge"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    
    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.buildcfg}")

    files {
        "src/**.cpp",
        "src/**.h"
    }

    includedirs {
        "src"
    }

    links {
        "sfml-system",
        "sfml-window",
        "sfml-graphics",
        "sfml-audio",
        "sfml-network",

        "imgui",
        "imgui-sfml",

        "fmt",
        "nativefiledialog-extended",
        "tomlplusplus"
    }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter "system:windows"
        vsprops { VcpkgEnableManifest = "true" }
        characterset "MBCS"
        linkoptions { "/IGNORE:4099" }
        buildoptions { "/bigobj", "/utf-8" }