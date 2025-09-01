project "Sandbox"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "on"

    targetdir ("%{wks.location}/Build/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/Build/Intermediates/" .. outputdir .. "/%{prj.name}")

    files
    {
        "Source/**.h",
        "Source/**.cpp"
    }

    includedirs
    {
        "%{wks.location}/Engine/Source",
        "%{wks.location}/Engine/Vendor/spdlog/include",
        "%{wks.location}/Engine/Vendor/SDL3/install/%{cfg.buildcfg}/include",
        "%{wks.location}/Engine/Vendor/SDL3/install/include",
        "%{wks.location}/Engine/Vendor/GLAD/generated/include",
        "%{wks.location}/Engine/Vendor/glm",
        "%{wks.location}/Engine/Vendor/nlohmann_json/single_include"
    }

    libdirs
    {
        "%{wks.location}/Engine/Vendor/SDL3/install/Debug/lib",
        "%{wks.location}/Engine/Vendor/SDL3/install/Release/lib",
    }

    links
    {
        "Engine"
    }

    filter "system:windows"
        buildoptions { "/utf-8" }

    filter "system:windows"
        -- Copy the Config directory next to the executable after build
        postbuildcommands
        {
            -- Ensure destination exists and copy recursively (quiet)
            'xcopy /E /I /Y "%{wks.location}\\Config" "%{cfg.targetdir}\\Config\\" >nul'
        }

    filter "system:linux"
        -- Copy the Config directory next to the executable after build
        postbuildcommands
        {
            -- Ensure destination exists and copy recursively
            'mkdir -p "%{cfg.targetdir}/Config" && cp -r "%{wks.location}/Config"/* "%{cfg.targetdir}/Config/"'
        }

    defines
    {
        "SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE"  -- Enable all log levels at compile time
    }

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "VX_PLATFORM_WINDOWS",
            "VX_OPENGL_SUPPORT",
            "VX_USE_SDL"
        }

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
            "dbghelp"
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

        filter "system:windows"
            libdirs
            {
                "%{wks.location}/Engine/Vendor/SDL3/install/Debug/lib"
            }
            links
            {
                "SDL3-static"
            }

        filter "system:linux"
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

        filter "system:windows"
            libdirs
            {
                "%{wks.location}/Engine/Vendor/SDL3/install/Release/lib"
            }
            links
            {
                "SDL3-static"
            }

        filter "system:linux"
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

        filter "system:windows"
            libdirs
            {
                "%{wks.location}/Engine/Vendor/SDL3/install/Release/lib"
            }
            links
            {
                "SDL3-static"
            }

        filter "system:linux"
            libdirs
            {
                "%{wks.location}/Engine/Vendor/SDL3/install/lib"
            }
            links
            {
                "SDL3"
            }