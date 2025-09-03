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
        "Vendor/SPIRV-Headers/include",
        "Vendor/shaderc/libshaderc/include",
        "Vendor/SPIRV-Cross"
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
            local vulkanFound = false
            
            -- Method 1: Use system VULKAN_SDK environment variable
            if vulkanSDK and os.isdir(vulkanSDK .. "/Include") then
                print("Engine: Using system Vulkan SDK headers: " .. vulkanSDK)
                includedirs { vulkanSDK .. "/Include" }
                vulkanFound = true
            end
            
            -- Method 2: Use local Vulkan SDK headers
            if not vulkanFound and os.isdir("../Vendor/VulkanSDK/Include") then
                print("Engine: Using local Vulkan SDK headers")
                includedirs { "../Vendor/VulkanSDK/Include" }
                vulkanFound = true
            end
            
            if not vulkanFound then
                print("Engine: WARNING - No Vulkan headers found!")
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
            "-Wpedantic",
            "-Wno-dangling-reference"  -- Disable dangling reference warning for spdlog compatibility
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
        defines { "VX_DEBUG", "VX_SPIRV_CROSS_AVAILABLE" }
        runtime "Debug"
        symbols "on"

        includedirs
        {
            "Vendor/SDL3/install/Debug/include",
            "Vendor/SDL3/install/include",
            "Vendor/shaderc/install/Debug/include",
            "Vendor/SPIRV-Tools/install/Debug/include",
            "Vendor/SPIRV-Cross/install/Debug/include"
        }

        filter "system:linux"
            libdirs
            {
                "Vendor/SDL3/install/lib"
            }

    filter "configurations:Release"
        defines { "VX_RELEASE", "VX_SPIRV_CROSS_AVAILABLE" }
        runtime "Release"
        optimize "on"

        includedirs
        {
            "Vendor/SDL3/install/Release/include",
            "Vendor/SDL3/install/include",
            "Vendor/shaderc/install/Release/include",
            "Vendor/SPIRV-Tools/install/Release/include",
            "Vendor/SPIRV-Cross/install/Release/include"
        }

        filter "system:linux"
            libdirs
            {
                "Vendor/SDL3/install/lib"
            }

    filter "configurations:Dist"
        defines { "VX_DIST", "VX_SPIRV_CROSS_AVAILABLE" }
        runtime "Release"
        optimize "on"

        includedirs
        {
            "Vendor/SDL3/install/Release/include",
            "Vendor/SDL3/install/include",
            "Vendor/shaderc/install/Release/include",
            "Vendor/SPIRV-Tools/install/Release/include",
            "Vendor/SPIRV-Cross/install/Release/include"
        }

        filter "system:linux"
            libdirs
            {
                "Vendor/SDL3/install/lib"
            }