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
        
        -- Add Vulkan SDK include if available. Avoid adding SDK libdir globally to prevent accidentally
        -- linking SDK's third-party libs (shaderc/glslang/spirv) with mismatched runtimes.
        filter { "system:windows", "configurations:*" }
            local vulkanSDK = os.getenv("VULKAN_SDK")
            if vulkanSDK then
                includedirs { vulkanSDK .. "/Include" }
                -- Link Vulkan loader explicitly via absolute path
                links { vulkanSDK .. "/Lib/vulkan-1.lib" }
            else
                -- Fallback to local Vulkan SDK
                includedirs { "%{wks.location}/Vendor/VulkanSDK/Include" }
                links { "%{wks.location}/Vendor/VulkanSDK/Lib/vulkan-1.lib" }
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

        -- Copy the Config directory next to the executable after build
        postbuildcommands
        {
            -- Ensure destination exists and copy recursively (quiet)
            'xcopy /E /I /Y "%{wks.location}\\Config" "%{cfg.targetdir}\\Config\\" >nul'
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
            "pthread"
        }

        -- Copy the Config directory next to the executable after build
        postbuildcommands
        {
            'mkdir -p "%{cfg.targetdir}/Config"',
            'cp -r "%{wks.location}/Config/"* "%{cfg.targetdir}/Config/"'
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
                "%{wks.location}/Engine/Vendor/SPIRV-Tools/install/Debug/lib"
            }

        filter { "system:linux", "configurations:Debug" }
            libdirs
            {
                "%{wks.location}/Engine/Vendor/SDL3/install/lib"
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
                "%{wks.location}/Engine/Vendor/SPIRV-Tools/install/Release/lib"
            }

        filter { "system:linux", "configurations:Release" }
            libdirs
            {
                "%{wks.location}/Engine/Vendor/SDL3/install/lib"
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
                "%{wks.location}/Engine/Vendor/SPIRV-Tools/install/Release/lib"
            }

        filter { "system:linux", "configurations:Dist" }
            libdirs
            {
                "%{wks.location}/Engine/Vendor/SDL3/install/lib"
            }
            links
            {
                "SDL3"
            }
