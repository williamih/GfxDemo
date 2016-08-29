workspace "GfxDemo"
    configurations { "Debug", "Release" }
    platforms { "OSX" }

function CreateProject(apiName, apiDefine, frameworks)
    project("GfxDemo-"..apiName)

        kind "WindowedApp"
        language "C++"
        targetdir("bin/%{cfg.buildcfg}/"..apiName)

        defines(apiDefine)

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
            files { "Source/**.m", "Source/**.mm", "Mac/**.xib", "Mac/**.strings", "Mac/Info.plist" }
            architecture "x64"
            links {
                "Cocoa.framework",
                "Carbon.framework",
                "QuartzCore.framework",
            }
            if frameworks ~= nil then
                links(frameworks)
            end
            buildoptions { "-std=c++14" }
            postbuildcommands {
                'rm -f %{cfg.buildtarget.directory}/Assets',
                'ln -s "${SRCROOT}/Assets" %{cfg.buildtarget.directory}/Assets',
            }

        filter "files:**.xib"
            buildaction "Resource"
        filter "files:**.plist"
            buildaction "Resource"
        filter "files:**.strings"
            buildaction "Resource"

end

CreateProject("Metal", "GPUDEVICE_API_METAL", "Metal.framework")
CreateProject("Null", "GPUDEVICE_API_NULL")
