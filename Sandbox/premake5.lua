project "Sandbox"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "on"

    targetdir ("%{wks.location}/Build/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/Build/Intermediates/" .. outputdir .. "/%{prj.name}")

    -- Global files
    files
    {
        "Source/**.h",
        "Source/**.cpp"
    }

    -- Global include directories
    includedirs
    {
        "%{wks.location}/Engine/Source",
        "%{wks.location}/Engine/Vendor/spdlog/include",
        "%{wks.location}/Engine/Vendor/GLAD/generated/include",
        "%{wks.location}/Engine/Vendor/glm",
        "%{wks.location}/Engine/Vendor/nlohmann_json/single_include",
        "%{wks.location}/Engine/Vendor/SPIRV-Headers/include"
    }

    -- Global links
    links
    {
        "Engine"
    }

    -- Global defines
    defines
    {
        "SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE"  -- Enable all log levels at compile time
    }

    filter "system:windows"
        systemversion "latest"
        buildoptions { "/utf-8" }

        defines
        {
            "VX_PLATFORM_WINDOWS",
            "VX_OPENGL_SUPPORT",
            "VX_USE_SDL"
        }
        
        -- Add Vulkan SDK include and linking. Try multiple approaches in order of preference.
        filter { "system:windows", "configurations:*" }
            local vulkanSDK = os.getenv("VULKAN_SDK")
            local vulkanFound = false
            
            -- Method 1: Use system VULKAN_SDK environment variable
            if vulkanSDK and os.isfile(vulkanSDK .. "/Lib/vulkan-1.lib") then
                print("Using system Vulkan SDK: " .. vulkanSDK)
                includedirs { vulkanSDK .. "/Include" }
                links { vulkanSDK .. "/Lib/vulkan-1.lib" }
                vulkanFound = true
            end
            
            -- Method 2: Check for local Vulkan SDK with proper lib
            if not vulkanFound and os.isfile("%{wks.location}/Vendor/VulkanSDK/Lib/vulkan-1.lib") then
                print("Using local Vulkan SDK with static lib")
                includedirs { "%{wks.location}/Vendor/VulkanSDK/Include" }
                links { "%{wks.location}/Vendor/VulkanSDK/Lib/vulkan-1.lib" }
                vulkanFound = true
            end
            
            -- Method 3: Use headers-only setup with dynamic loading (fallback)
            if not vulkanFound then
                print("Using Vulkan headers-only setup (dynamic loading)")
                includedirs { "%{wks.location}/Vendor/VulkanSDK/Include" }
                defines { "VK_USE_PLATFORM_WIN32_KHR" }
                -- Don't link vulkan-1.lib, assume dynamic loading at runtime
            end

        links
        {
            -- Windows system libraries required by SDL3
            "winmm",
            "version",
            "imm32",
            "setupapi",
            "shell32",
            "user32",
            "gdi32",
            "opengl32",
            "ole32",
            "oleaut32",
            "advapi32",
            -- Required for CrashHandler
            "dbghelp",
            -- Config-agnostic logical names; resolved via libdirs below
            "SDL3-static",
            "shaderc_combined"
        }

        -- Copy the Config and Assets directories next to the executable after build
        postbuildcommands
        {
            -- Ensure destination exists and copy recursively (quiet)
            'xcopy /E /I /Y "%{wks.location}\\Config" "%{cfg.targetdir}\\Config\\" >nul',
            'xcopy /E /I /Y "%{wks.location}\\Assets" "%{cfg.targetdir}\\Assets\\" >nul'
        }

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
            "pthread",
            "shaderc_combined"
        }

        -- Copy the Config and Assets directories next to the executable after build
        postbuildcommands
        {
            'mkdir -p "%{cfg.targetdir}/Config"',
            'cp -r "%{wks.location}/Config/"* "%{cfg.targetdir}/Config/"',
            'mkdir -p "%{cfg.targetdir}/Assets"',
            'cp -r "%{wks.location}/Assets/"* "%{cfg.targetdir}/Assets/"'
        }

    filter "configurations:Debug"
        defines "VX_DEBUG"
        runtime "Debug"
        symbols "on"

        includedirs
        {
            "%{wks.location}/Engine/Vendor/SDL3/install/Debug/include",
            "%{wks.location}/Engine/Vendor/SDL3/install/include",
            "%{wks.location}/Engine/Vendor/shaderc/install/Debug/include",
            "%{wks.location}/Engine/Vendor/SPIRV-Tools/install/Debug/include"
        }

        filter { "system:windows", "configurations:Debug" }
            libdirs
            {
                "%{wks.location}/Engine/Vendor/SDL3/install/Debug/lib",
                "%{wks.location}/Engine/Vendor/shaderc/install/Debug/lib",
                "%{wks.location}/Engine/Vendor/SPIRV-Tools/install/Debug/lib",
                "%{wks.location}/Engine/Vendor/SPIRV-Cross/install/Debug/lib"
            }
            
            links
            {
                -- Debug versions of SPIRV-Cross libraries (with 'd' suffix)
                "spirv-cross-cored",
                "spirv-cross-glsld",
                "spirv-cross-utild"
            }

        filter { "system:linux", "configurations:Debug" }
            libdirs
            {
                "%{wks.location}/Engine/Vendor/SDL3/install/lib",
                "%{wks.location}/Engine/Vendor/shaderc/install/lib"
            }
            links
            {
                "SDL3"
            }

    filter "configurations:Release"
        defines "VX_RELEASE"
        runtime "Release"
        optimize "on"

        includedirs
        {
            "%{wks.location}/Engine/Vendor/SDL3/install/Release/include",
            "%{wks.location}/Engine/Vendor/SDL3/install/include",
            "%{wks.location}/Engine/Vendor/shaderc/install/Release/include",
            "%{wks.location}/Engine/Vendor/SPIRV-Tools/install/Release/include"
        }

        filter { "system:windows", "configurations:Release" }
            libdirs
            {
                "%{wks.location}/Engine/Vendor/SDL3/install/Release/lib",
                "%{wks.location}/Engine/Vendor/shaderc/install/Release/lib",
                "%{wks.location}/Engine/Vendor/SPIRV-Tools/install/Release/lib",
                "%{wks.location}/Engine/Vendor/SPIRV-Cross/install/Release/lib"
            }
            
            links
            {
                -- Release versions of SPIRV-Cross libraries (no suffix)
                "spirv-cross-core",
                "spirv-cross-glsl",
                "spirv-cross-util"
            }

        filter { "system:linux", "configurations:Release" }
            libdirs
            {
                "%{wks.location}/Engine/Vendor/SDL3/install/lib",
                "%{wks.location}/Engine/Vendor/shaderc/install/lib"
            }
            links
            {
                "SDL3"
            }

    filter "configurations:Dist"
        defines "VX_DIST"
        runtime "Release"
        optimize "on"

        includedirs
        {
            "%{wks.location}/Engine/Vendor/SDL3/install/Release/include",
            "%{wks.location}/Engine/Vendor/SDL3/install/include",
            "%{wks.location}/Engine/Vendor/shaderc/install/Release/include",
            "%{wks.location}/Engine/Vendor/SPIRV-Tools/install/Release/include"
        }

        filter { "system:windows", "configurations:Dist" }
            libdirs
            {
                "%{wks.location}/Engine/Vendor/SDL3/install/Release/lib",
                "%{wks.location}/Engine/Vendor/shaderc/install/Release/lib",
                "%{wks.location}/Engine/Vendor/SPIRV-Tools/install/Release/lib",
                "%{wks.location}/Engine/Vendor/SPIRV-Cross/install/Release/lib"
            }
            
            links
            {
                -- Dist uses Release libraries (no suffix)
                "spirv-cross-core",
                "spirv-cross-glsl",
                "spirv-cross-util"
            }

        filter { "system:linux", "configurations:Dist" }
            libdirs
            {
                "%{wks.location}/Engine/Vendor/SDL3/install/lib",
                "%{wks.location}/Engine/Vendor/shaderc/install/lib"
            }
            links
            {
                "SDL3"
            }
