cmake_minimum_required(VERSION 3.21)
project(Jello)

# Default to Release for single-config generators (MinGW Makefiles, Ninja, Unix Makefiles)
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/release)

# SFML 3: use an installed copy if there is one, otherwise download and build it
option(JELLO_FETCH_SFML "Download SFML 3 when it is not installed" ON)

find_package(SFML 3 QUIET COMPONENTS Graphics Window System Audio)

if(SFML_FOUND)
    message(STATUS "Using installed SFML ${SFML_VERSION}")
else()
    if(NOT JELLO_FETCH_SFML)
        message(FATAL_ERROR
            "SFML 3 was not found and JELLO_FETCH_SFML is OFF.\n"
            "Either install SFML 3.0 (Graphics, Window, System, Audio) and point CMake to it with\n"
            "  -DCMAKE_PREFIX_PATH=<SFML install dir>\n"
            "or re-run CMake with -DJELLO_FETCH_SFML=ON to download and build it automatically.")
    endif()

    message(STATUS "SFML 3 not found: downloading and building SFML 3.0.0 (first build takes several minutes)")

    # Static SFML so the executable does not need any SFML DLLs
    set(BUILD_SHARED_LIBS OFF)
    set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)

    # SFML 3.0.0 still declares an old cmake_minimum_required; CMake 4 needs this
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

    include(FetchContent)
    FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.0.0
        GIT_SHALLOW ON
    )
    FetchContent_MakeAvailable(SFML)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/source)

# Source files
set(SOURCES
    source/main.cpp
    source/Data/Resources/ResourceManager.cpp
    source/Data/Resources/Audio.cpp
    source/Data/Logging/Logger.cpp
    source/Data/Config/UIConfig.cpp
    source/Presentation/Core/GameWindow.cpp
    source/Presentation/Core/GameCoordinator.cpp
    source/Presentation/Core/GameSystemInitializer.cpp
    source/Presentation/Core/GameCallbackManager.cpp
    source/Presentation/Core/WindowManager.cpp
    source/Presentation/Input/InputHandler.cpp
    source/Presentation/Input/KeyBindingFactory.cpp
    source/Presentation/Rendering/Renderer.cpp
    source/Presentation/Rendering/BackgroundManager.cpp
    source/Presentation/Rendering/CameraManager.cpp
    source/Presentation/Rendering/ParticleSystem.cpp
    source/Presentation/UI/MenuUI.cpp
    source/Presentation/UI/UIManager.cpp
    source/Presentation/UI/UIComponents.cpp
    source/Presentation/UI/UIServices.cpp
    source/Presentation/Game/UiKit.cpp
    source/Presentation/Game/GameAudio.cpp
    source/Presentation/Game/Jello.cpp
    source/Presentation/Game/PipeField.cpp
    source/Presentation/Game/Scene.cpp
    source/Presentation/Game/Effects.cpp
    source/Presentation/Game/Hud.cpp
    source/Presentation/Game/ReadyOverlay.cpp
    source/Presentation/Game/PauseOverlay.cpp
    source/Presentation/Game/GameOverOverlay.cpp
    source/Presentation/Game/BounceGame.cpp
    source/Presentation/Game/Toast.cpp
    source/Presentation/Game/StackGame.cpp
    source/Presentation/Game/MenuMascot.cpp
    source/Presentation/Panels/TrophiesPanel.cpp
    source/Presentation/Game/ArcadeGame.cpp
    source/Presentation/Game/SmashGame.cpp
    source/Presentation/Game/BlasterGame.cpp
    source/Presentation/Game/SlitherGame.cpp
    source/Presentation/Panels/GamePickerPanel.cpp
    source/Data/Profile/UserData.cpp
    source/Presentation/Panels/Widgets.cpp
    source/Presentation/Panels/SettingsPanel.cpp
    source/Presentation/Panels/InfoPanels.cpp
    source/Presentation/Panels/AccountPanel.cpp
    source/Presentation/Panels/PanelManager.cpp
)

# Create executable
add_executable(Jello ${SOURCES})

# Link SFML libraries (imported targets from an installed SFML or aliases from FetchContent)
target_link_libraries(Jello SFML::Graphics SFML::Window SFML::System SFML::Audio)

# MinGW: link the C++ and GCC runtimes statically so the exe runs without MSYS2 on PATH
if(MINGW)
    target_link_options(Jello PRIVATE -static -static-libgcc -static-libstdc++)
endif()

# Copy assets to output directory
file(COPY ${CMAKE_SOURCE_DIR}/source/Data/Resources/assets DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY ${CMAKE_SOURCE_DIR}/source/Data/Config DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY ${CMAKE_SOURCE_DIR}/source/Presentation/Input/Keybinds.json DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Set working directory for debugging
set_target_properties(Jello PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

# Copy SFML runtime DLLs next to the executable when SFML is built as shared libs
get_target_property(JELLO_SFML_GRAPHICS_TYPE SFML::Graphics TYPE)
if(WIN32 AND JELLO_SFML_GRAPHICS_TYPE STREQUAL "SHARED_LIBRARY")
    add_custom_command(TARGET Jello POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:Jello> $<TARGET_FILE_DIR:Jello>
        COMMAND_EXPAND_LISTS)
endif()

# Print build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
