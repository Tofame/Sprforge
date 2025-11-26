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
            "Foundation.framework"
        }
        buildoptions { "-x objective-c++" }

    filter {}

    -- Libraries resolved via vcpkg manifest
    links {
        "glfw3dll",
        "glad",
        "tomlplusplus",
        "nfd"
    }

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
    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"
        kind "ConsoleApp"
        linkoptions { "/SUBSYSTEM:CONSOLE" }
		links {
			"imguid",
			"fmtd"
		}

    filter "configurations:Release"
        defines { "NDEBUG" }
        runtime "Release"
        optimize "On"
        kind "WindowedApp"
        linkoptions { "/SUBSYSTEM:WINDOWS" }
		links {
			"imgui",
			"fmt"
		}

	filter {}