local osID = os.get()

workspace "GfxDemo"
    configurations { "Debug", "Release" }
    if osID == "windows" then
        platforms { "Windows" }
    elseif osID == "macosx" then
        platforms { "OSX " }
    end

function CreateProject(apiName, apiDefine, listOfLinks)
    project("GfxDemo-"..apiName)

        kind "WindowedApp"
        language "C++"
        targetdir("bin/%{cfg.buildcfg}/"..apiName)

        defines(apiDefine)

        files { "Source/**.h", "Source/**.c", "Source/**.cpp" }

        includedirs "Source"

        if listOfLinks ~= nil then
            links(listOfLinks)
        end

        filter "configurations:Debug"
            defines {
                "DEBUG",
                "GPUDEVICE_DEBUG_MODE"
            }
            flags { "Symbols" }

        filter "configurations:Release"
            defines { "NDEBUG" }
            optimize "On"

        filter "platforms:Windows"
            architecture "x64"
            postbuildcommands {
                'if exist "%{cfg.buildtarget.directory}Assets" del /q "%{cfg.buildtarget.directory}Assets";',
                'mklink "%{cfg.buildtarget.directory}Assets" "$(SolutionDir)Assets";',
            }

        filter "platforms:OSX"
            files { "Source/**.m", "Source/**.mm", "Mac/**.xib", "Mac/**.strings", "Mac/Info.plist" }
            architecture "x64"
            links {
                "Cocoa.framework",
                "Carbon.framework",
                "QuartzCore.framework",
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
        filter "files:**.strings"
            buildaction "Resource"

end

if osID == "macosx" then
    CreateProject("Metal", "GPUDEVICE_API_METAL", "Metal.framework")
end
CreateProject("Null", "GPUDEVICE_API_NULL")
