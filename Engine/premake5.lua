project "Engine"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "on"

    targetdir ("%{wks.location}/Build/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/Build/Intermediates/" .. outputdir .. "/%{prj.name}")

    pchheader "vxpch.h"
    pchsource "Source/vxpch.cpp"

    -- Disable PCH and enforce C compilation for GLAD's generated C source
    filter "files:Vendor/GLAD/generated/src/gl.c"
        flags { "NoPCH" }
        compileas "C"
        disablewarnings { "4005" }
    filter {}

    files
    {
        "Source/**.h",
        "Source/**.cpp",
        "Vendor/GLAD/generated/src/gl.c"
    }

    defines
    {
        "_CRT_SECURE_NO_WARNINGS",
    }

    includedirs
    {
        "Source",
        "Vendor/spdlog/include",
        "Vendor/GLAD/generated/include",
        "Vendor/glm",
        "Vendor/nlohmann_json/single_include",
        "Vendor/SPIRV-Headers/include"
    }

    filter "system:windows"
        buildoptions { "/utf-8" }
        systemversion "latest"

        defines
        {
            "VX_PLATFORM_WINDOWS",
            "VX_OPENGL_SUPPORT",
            "VX_USE_SDL"
        }
        
        filter { "system:windows", "configurations:*" }
            local vulkanSDK = os.getenv("VULKAN_SDK")
            if vulkanSDK then
                includedirs { vulkanSDK .. "/Include" }
            else
                -- Fallback to local Vulkan SDK headers only
                includedirs { "../Vendor/VulkanSDK/Include" }
            end

    filter "system:linux"
        defines
        {
            "VX_PLATFORM_LINUX",
            "VX_OPENGL_SUPPORT",
            "VX_USE_SDL"
        }

        buildoptions
        {
            "-Wall",
            "-Wextra",
            "-Wpedantic"
        }

        links
        {
            "SDL3",
            "GL",
            "GLU",
            "X11",
            "Xrandr",
            "Xinerama",
            "Xcursor",
            "Xi",
            "Xext",
            "asound",
            "pulse",
            "dbus-1",
            "udev",
            "usb-1.0",
            "pthread"
        }

    filter "configurations:Debug"
        defines "VX_DEBUG"
        runtime "Debug"
        symbols "on"

        includedirs
        {
            "Vendor/SDL3/install/Debug/include",
            "Vendor/SDL3/install/include",
            "Vendor/shaderc/install/Debug/include",
            "Vendor/SPIRV-Tools/install/Debug/include"
        }

        filter "system:linux"
            libdirs
            {
                "Vendor/SDL3/install/lib"
            }

    filter "configurations:Release"
        defines "VX_RELEASE"
        runtime "Release"
        optimize "on"

        includedirs
        {
            "Vendor/SDL3/install/Release/include",
            "Vendor/SDL3/install/include",
            "Vendor/shaderc/install/Release/include",
            "Vendor/SPIRV-Tools/install/Release/include"
        }

        filter "system:linux"
            libdirs
            {
                "Vendor/SDL3/install/lib"
            }

    filter "configurations:Dist"
        defines "VX_DIST"
        runtime "Release"
        optimize "on"

        includedirs
        {
            "Vendor/SDL3/install/Release/include",
            "Vendor/SDL3/install/include",
            "Vendor/shaderc/install/Release/include",
            "Vendor/SPIRV-Tools/install/Release/include"
        }

        filter "system:linux"
            libdirs
            {
                "Vendor/SDL3/install/lib"
            }