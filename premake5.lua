workspace "Vortex"
    architecture "x86_64"
    startproject "Sandbox"

    configurations
    {
        "Debug",
        "Release",
        "Dist"
    }

    flags
    {
        "MultiProcessorCompile"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
    -- Include vendor projects here when added
group ""

include "Engine"
include "Sandbox"
