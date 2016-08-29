workspace "GfxDemo"
    configurations { "Debug", "Release" }
    platforms { "OSX" }

project "GfxDemo"
    kind "WindowedApp"
    language "C++"
    targetdir "bin/%{cfg.buildcfg}"

    files { "Source/**.h", "Source/**.c", "Source/**.cpp" }

    includedirs "Source"

    filter "configurations:Debug"
        defines {
            "DEBUG",
            "GPUDEVICE_DEBUG_MODE"
        }
        flags { "Symbols" }

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter "platforms:OSX"
        files { "Source/**.m", "Source/**.mm", "Mac/**.xib", "Mac/Info.plist" }
        architecture "x64"
        links {
            "Cocoa.framework",
            "Carbon.framework",
            "Metal.framework",
            "QuartzCore.framework"
        }
        buildoptions { "-std=c++14" }
        postbuildcommands {
            'rm -f %{cfg.buildtarget.directory}/Assets',
            'ln -s "${SRCROOT}/Assets" %{cfg.buildtarget.directory}/Assets',
        }

    filter "files:**.xib"
        buildaction "Resource"
    filter "files:**.plist"
        buildaction "Resource"
