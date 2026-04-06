cmake_minimum_required(VERSION 3.14.0)

option(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES "Enable VST 3 Plug-in Examples" OFF)
option(SMTG_ENABLE_VST3_HOSTING_EXAMPLES "Enable VST 3 Hosting Examples" OFF)

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13 CACHE STRING "")

if(APPLE)
    set(SMTG_AUDIOUNIT_SDK_PATH "$ENV{SMTG_AUDIOUNIT_SDK_PATH}" CACHE PATH "Path to the AudioUnit SDK" FORCE)
    set(SMTG_COREAUDIO_SDK_PATH "$ENV{SMTG_COREAUDIO_SDK_PATH}" CACHE PATH "Here you can define where to find the COREAUDIO SDK" FORCE)
endif()

if(APPLE)
    # libc++ deprecated std::wstring_convert/std::codecvt* and some SDK code still uses it.
    # If the build treats deprecations as errors, disable libc++ deprecation attributes globally.
    add_compile_definitions(_LIBCPP_DISABLE_DEPRECATION_WARNINGS)
endif()

set(vst3sdk_SOURCE_DIR "$ENV{VST3SDK_ROOT}")
if(NOT vst3sdk_SOURCE_DIR)
    message(FATAL_ERROR "Path to VST3 SDK is empty!")
endif()

project(PEU_1
    # This is your plug-in version number. Change it here only.
    # Version number symbols usable in C++ can be found in
    # source/version.h and ${PROJECT_BINARY_DIR}/projectversion.h.
    VERSION 0.1.1.0
    DESCRIPTION "PEU_1 VST 3 Plug-in"
)

set(SMTG_VSTGUI_ROOT "${vst3sdk_SOURCE_DIR}")

add_subdirectory(${vst3sdk_SOURCE_DIR} ${PROJECT_BINARY_DIR}/vst3sdk)
smtg_enable_vst3_sdk()

if(APPLE)
    target_compile_definitions(vstgui PRIVATE _LIBCPP_DISABLE_DEPRECATION_WARNINGS)
    target_compile_options(vstgui PRIVATE
        -Wno-deprecated-declarations
        -Wno-error=deprecated-declarations
    )
endif()

smtg_add_vst3plugin(PEU_1
    source/version.h
    source/Peu1cids.h
    source/Peu1processor.h
    source/Peu1processor.cpp
    source/Peu1controller.h
    source/Peu1controller.cpp
    source/Peu1entry.cpp
)

set_target_properties(PEU_1 PROPERTIES
    OUTPUT_NAME "PEU_1"
)

#- VSTGUI Wanted ----
if(SMTG_ENABLE_VSTGUI_SUPPORT)
    target_sources(PEU_1
        PRIVATE
            resource/Peu1editor.uidesc
    )
    target_link_libraries(PEU_1
        PRIVATE
            vstgui_support
    )
    smtg_target_add_plugin_resources(PEU_1
        RESOURCES
            "resource/Peu1editor.uidesc"
    )
endif(SMTG_ENABLE_VSTGUI_SUPPORT)
# -------------------

smtg_target_add_plugin_snapshots (PEU_1
    RESOURCES
        resource/CFBFD7B17EA055A5B270DCC1A2012A5C_snapshot.png
        resource/CFBFD7B17EA055A5B270DCC1A2012A5C_snapshot_2.0x.png
)

target_link_libraries(PEU_1
    PRIVATE
        sdk
)

smtg_target_configure_version_file(PEU_1)

if(SMTG_MAC AND XCODE AND SMTG_ENABLE_AUV2_BUILDS)
    list(APPEND CMAKE_MODULE_PATH "${vst3sdk_SOURCE_DIR}/cmake/modules")
    include(SMTG_AddVST3AuV2)
    smtg_target_add_auv2(PEU_1_AU
        BUNDLE_NAME PEU-1
        BUNDLE_IDENTIFIER com.marinedalek.peu1.audiounit
        INFO_PLIST_TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/resource/au-info.plist
        VST3_PLUGIN_TARGET PEU_1)
endif()

if(SMTG_MAC)
    smtg_target_set_bundle(PEU_1
        BUNDLE_IDENTIFIER com.marinedalek.peu1
        COMPANY_NAME "marinedalek"
    )
    smtg_target_set_debug_executable(PEU_1
        "/Applications/VST3PluginTestHost.app"
        "--pluginfolder;$(BUILT_PRODUCTS_DIR)"
    )
elseif(SMTG_WIN)
    target_sources(PEU_1 PRIVATE
        resource/win32resource.rc
    )
    if(MSVC)
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT PEU_1)

        smtg_target_set_debug_executable(PEU_1
            "$(ProgramW6432)/Steinberg/VST3PluginTestHost/VST3PluginTestHost.exe"
            "--pluginfolder \"$(OutDir)/\""
        )
    endif()
endif(SMTG_MAC)
