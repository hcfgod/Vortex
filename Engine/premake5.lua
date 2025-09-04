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
        -- Suppress macro redefinition and code analysis warnings from GLAD C source
        disablewarnings { "4005", "6001" }
        
    -- Disable PCH for SPIRV-Cross source files to avoid conflicts
    filter "files:Vendor/SPIRV-Cross/**.cpp"
        flags { "NoPCH" }
        -- Suppress typical build and code analysis warnings from external SPIRV-Cross
        disablewarnings { "4996", "4244", "4267", "6001", "6011" }
    filter {}

    files
    {
        "Source/**.h",
        "Source/**.cpp",
        "Vendor/GLAD/generated/src/gl.c",
        -- SPIRV-Cross core + GLSL backend + required support files
        "Vendor/SPIRV-Cross/spirv_cross.cpp",
        "Vendor/SPIRV-Cross/spirv_cross_util.cpp",
        "Vendor/SPIRV-Cross/spirv_cross_parsed_ir.cpp",
        "Vendor/SPIRV-Cross/spirv_parser.cpp",
        "Vendor/SPIRV-Cross/spirv_cfg.cpp",
        "Vendor/SPIRV-Cross/spirv_reflect.cpp",
        "Vendor/SPIRV-Cross/spirv_glsl.cpp"
    }

    defines
    {
        "_CRT_SECURE_NO_WARNINGS",
    }

    includedirs
    {
        "Source"
    }

    -- Mark vendor headers as external to silence their warnings (fmt/spdlog, glm, json, etc.)
    externalincludedirs
    {
        "Vendor/spdlog/include",
        "Vendor/GLAD/generated/include",
        "Vendor/glm",
        "Vendor/nlohmann_json/single_include",
        "Vendor/SPIRV-Headers/include",
        "Vendor/shaderc/libshaderc/include",
        "Vendor/SPIRV-Cross"
    }

    -- Disable warnings from external headers
    externalwarnings "Off"

    filter "system:windows"
        -- Ensure MSVC does not run code analysis on external headers and keeps their warnings at W0
        buildoptions { "/utf-8", "/analyze:external-" }
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

        externalincludedirs
        {
            "Vendor/SDL3/install/Debug/include",
            "Vendor/SDL3/install/include",
            "Vendor/shaderc/install/Debug/include",
            "Vendor/SPIRV-Tools/install/Debug/include",
            "Vendor/SPIRV-Cross/install/Debug/include"
        }

        filter { "system:windows", "configurations:Debug" }
            libdirs
            {
                "Vendor/SDL3/install/Debug/lib",
                "Vendor/shaderc/install/Debug/lib",
                "Vendor/SPIRV-Tools/install/Debug/lib",
                "Vendor/SPIRV-Cross/install/Debug/lib"
            }
            
            -- SPIRV-Cross libraries removed - building from source now

        filter "system:linux"
            libdirs
            {
                "Vendor/SDL3/install/lib"
            }

    filter "configurations:Release"
        defines { "VX_RELEASE", "VX_SPIRV_CROSS_AVAILABLE" }
        runtime "Release"
        optimize "on"

        externalincludedirs
        {
            "Vendor/SDL3/install/Release/include",
            "Vendor/SDL3/install/include",
            "Vendor/shaderc/install/Release/include",
            "Vendor/SPIRV-Tools/install/Release/include",
            "Vendor/SPIRV-Cross/install/Release/include"
        }

        filter { "system:windows", "configurations:Release" }
            libdirs
            {
                "Vendor/SDL3/install/Release/lib",
                "Vendor/shaderc/install/Release/lib",
                "Vendor/SPIRV-Tools/install/Release/lib",
                "Vendor/SPIRV-Cross/install/Release/lib"
            }
            
            -- SPIRV-Cross libraries removed - building from source now

        filter "system:linux"
            libdirs
            {
                "Vendor/SDL3/install/lib"
            }

    filter "configurations:Dist"
        defines { "VX_DIST", "VX_SPIRV_CROSS_AVAILABLE" }
        runtime "Release"
        optimize "on"

        externalincludedirs
        {
            "Vendor/SDL3/install/Release/include",
            "Vendor/SDL3/install/include",
            "Vendor/shaderc/install/Release/include",
            "Vendor/SPIRV-Tools/install/Release/include",
            "Vendor/SPIRV-Cross/install/Release/include"
        }

        filter { "system:windows", "configurations:Dist" }
            libdirs
            {
                "Vendor/SDL3/install/Release/lib",
                "Vendor/shaderc/install/Release/lib",
                "Vendor/SPIRV-Tools/install/Release/lib",
                "Vendor/SPIRV-Cross/install/Release/lib"
            }
            
            -- SPIRV-Cross libraries removed - building from source now

        filter "system:linux"
            libdirs
            {
                "Vendor/SDL3/install/lib"
            }