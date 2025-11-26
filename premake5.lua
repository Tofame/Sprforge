-- Workspace configuration --------------------------------------------------
workspace "Sprforge"
    configurations { "Debug", "Release" }
    platforms { "x64" }
    startproject "Sprforge"
    location ""

    filter "platforms:x64"
        architecture "x86_64"

    filter {}

-- Project configuration ----------------------------------------------------
project "Sprforge"
    kind "WindowedApp"
    language "C++"
    cppdialect "C++20"
    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.buildcfg}")
    flags { "MultiProcessorCompile" }

    files {
        "src/**.cpp",
        "src/**.h"
    }
    

    defines { "IMGUI_IMPL_OPENGL_LOADER_GLAD" }

    -- vcpkg manifest integration (Visual Studio picks up vcpkg.json automatically)
    filter "system:windows"
        vsprops {
            VcpkgEnableManifest = "true"
        }
        buildoptions {"/utf-8" }
        defines { "NOMINMAX", "_WIN32", "WIN32", "_WINDOWS", "PLATFORM_WINDOWS" }
        linkoptions {"/SUBSYSTEM:WINDOWS", "/ENTRY:mainCRTStartup"}
        systemversion "latest"
        links { "opengl32", "gdi32", "user32", "shell32", "ole32" }

    filter "system:macosx"
        links {
            "AppKit.framework",
            "CoreGraphics.framework",
            "Foundation.framework",
            "OpenGL.framework",
            "IOKit.framework",
            "Cocoa.framework"
        }
        buildoptions { "-x objective-c++" }
        
        -- vcpkg integration for macOS
        if os.getenv("VCPKG_ROOT") then
            local vcpkg_root = os.getenv("VCPKG_ROOT")
            local triplet = os.getenv("VCPKG_DEFAULT_TRIPLET") or "x64-osx"
            includedirs { path.join(vcpkg_root, "installed", triplet, "include") }
            libdirs { path.join(vcpkg_root, "installed", triplet, "lib") }
            libdirs { path.join(vcpkg_root, "installed", triplet, "lib", "pkgconfig") }
        end

    filter "system:linux"
        links {
            "GL",
            "X11",
            "Xrandr",
            "Xi",
            "Xcursor",
            "Xinerama",
            "pthread",
            "dl"
        }
        
        -- vcpkg integration for Linux
        if os.getenv("VCPKG_ROOT") then
            local vcpkg_root = os.getenv("VCPKG_ROOT")
            local triplet = os.getenv("VCPKG_DEFAULT_TRIPLET") or "x64-linux"
            includedirs { path.join(vcpkg_root, "installed", triplet, "include") }
            libdirs { path.join(vcpkg_root, "installed", triplet, "lib") }
            libdirs { path.join(vcpkg_root, "installed", triplet, "lib", "pkgconfig") }
        end

    filter {}

    -- Libraries resolved via vcpkg manifest (Windows) or vcpkg (macOS/Linux)
    filter "system:windows"
        links {
            "glfw3dll",
            "glad",
            "tomlplusplus",
            "nfd"
        }
    
    filter "system:linux"
        links {
            "glfw3",
            "glad",
            "tomlplusplus",
            "nfd"
        }
    
    filter "system:macosx"
        links {
            "glfw",
            "glad",
            "tomlplusplus",
            "nfd"
        }
    
    filter {}

    -- Custom include/library overrides (optional)
    newoption {
        trigger     = "custom-includes",
        description = "Comma-separated list of additional include paths.",
        value       = "paths"
    }

    newoption {
        trigger     = "custom-libs",
        description = "Comma-separated list of additional library paths.",
        value       = "paths"
    }

    if _OPTIONS["custom-includes"] then
        includedirs(string.explode(_OPTIONS["custom-includes"], ","))
    end

    if _OPTIONS["custom-libs"] then
        libdirs(string.explode(_OPTIONS["custom-libs"], ","))
    end

    -- Build configuration settings
    filter { "configurations:Debug", "system:windows" }
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"
        kind "ConsoleApp"
        linkoptions { "/SUBSYSTEM:CONSOLE" }

    filter { "configurations:Debug", "system:macosx" }
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"
        kind "ConsoleApp"

    filter { "configurations:Debug", "system:linux" }
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"
        kind "ConsoleApp"

    filter "configurations:Debug"
        links {
            "imguid",
            "fmtd"
        }

    filter { "configurations:Release", "system:windows" }
        defines { "NDEBUG" }
        runtime "Release"
        optimize "On"
        kind "WindowedApp"
        linkoptions { "/SUBSYSTEM:WINDOWS" }

    filter { "configurations:Release", "system:macosx" }
        defines { "NDEBUG" }
        runtime "Release"
        optimize "On"
        kind "WindowedApp"

    filter { "configurations:Release", "system:linux" }
        defines { "NDEBUG" }
        runtime "Release"
        optimize "On"
        kind "WindowedApp"

    filter "configurations:Release"
        links {
            "imgui",
            "fmt"
        }

    filter {}