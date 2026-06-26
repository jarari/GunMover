-- include subprojects
includes("lib/commonlibf4")

-- set project constants
set_project("GunMover")
set_version("1.0.0")
set_license("MIT")
set_languages("c++23")
set_warnings("allextra")
set_encodings("utf-8")

-- add common rules
add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

add_defines("COMMONLIB_RUNTIMECOUNT=3")

-- add packages
add_requires("nlohmann_json v3.12.0")
add_requires("microsoft-detours")

-- define targets
target("GunMover")
    add_rules("commonlibf4.plugin", {
        name = "GunMover",
        author = "jarari",
        description = "Moves the gun in first person view.",
        plugin_template = path.join(os.projectdir(), "res/commonlibf4-plugin.cpp.in"),
    })

    -- add packages
    add_packages("nlohmann_json")
    add_packages("microsoft-detours")

    -- add src files
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")

    -- add definitions
    add_defines("_UNICODE")

    -- set precompiled header
    set_pcxxheader("src/PCH.h")
