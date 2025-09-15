project "Tests"
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
		"%{wks.location}/Engine/Vendor/doctest"
	}

	externalincludedirs
	{
		"%{wks.location}/Engine/Vendor/spdlog/include",
		"%{wks.location}/Engine/Vendor/GLAD/generated/include",
		"%{wks.location}/Engine/Vendor/glm",
		"%{wks.location}/Engine/Vendor/nlohmann_json/single_include",
		"%{wks.location}/Engine/Vendor/SPIRV-Headers/include",
		"%{wks.location}/Engine/Vendor/doctest"
	}

	externalwarnings "Off"

	links
	{
		"Engine"
	}

	filter "system:windows"
		systemversion "latest"
		buildoptions { "/utf-8", "/analyze:external-" }

		defines
		{
			"VX_PLATFORM_WINDOWS",
			"VX_OPENGL_SUPPORT",
			"VX_USE_SDL"
		}

		filter { "system:windows", "configurations:*" }
			local vulkanSDK = os.getenv("VULKAN_SDK")
			local vulkanFound = false
			if vulkanSDK and os.isfile(vulkanSDK .. "/Lib/vulkan-1.lib") then
				includedirs { vulkanSDK .. "/Include" }
				links { vulkanSDK .. "/Lib/vulkan-1.lib" }
				vulkanFound = true
			end
			if not vulkanFound and os.isfile("%{wks.location}/Vendor/VulkanSDK/Lib/vulkan-1.lib") then
				includedirs { "%{wks.location}/Vendor/VulkanSDK/Include" }
				links { "%{wks.location}/Vendor/VulkanSDK/Lib/vulkan-1.lib" }
				vulkanFound = true
			end
			if not vulkanFound then
				includedirs { "%{wks.location}/Vendor/VulkanSDK/Include" }
				defines { "VK_USE_PLATFORM_WIN32_KHR" }
			end

		links
		{
			"winmm","version","imm32","setupapi","shell32","user32","gdi32","opengl32","ole32","oleaut32","advapi32",
			"dbghelp","SDL3-static","shaderc_combined"
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
			"SDL3","GL","GLU","X11","Xrandr","Xinerama","Xcursor","Xi","Xext","asound","pulse","dbus-1","udev","usb-1.0","pthread","shaderc_combined"
		}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
		externalincludedirs
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
		filter { "system:linux", "configurations:Debug" }
			libdirs { "%{wks.location}/Engine/Vendor/SDL3/install/lib", "%{wks.location}/Engine/Vendor/shaderc/install/lib" }

	filter "configurations:Release"
		defines "VX_RELEASE"
		runtime "Release"
		optimize "on"
		externalincludedirs
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
		filter { "system:linux", "configurations:Release" }
			libdirs { "%{wks.location}/Engine/Vendor/SDL3/install/lib", "%{wks.location}/Engine/Vendor/shaderc/install/lib" }

	filter "configurations:Dist"
		defines "VX_DIST"
		runtime "Release"
		optimize "on"
		externalincludedirs
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
		filter { "system:linux", "configurations:Dist" }
			libdirs { "%{wks.location}/Engine/Vendor/SDL3/install/lib", "%{wks.location}/Engine/Vendor/shaderc/install/lib" }
