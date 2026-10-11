cmake_minimum_required(VERSION 3.16)
project(Jello)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/release)

# Find SFML
find_package(SFML 3.0 REQUIRED COMPONENTS Graphics Window System Audio)

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
    source/Presentation/UI/GameSelectionUI.cpp
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
    source/Presentation/Game/FlappyGame.cpp
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

# Link SFML libraries
target_link_libraries(Jello SFML::Graphics SFML::Window SFML::System SFML::Audio)

# Copy assets to output directory
file(COPY ${CMAKE_SOURCE_DIR}/source/Data/Resources/assets DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY ${CMAKE_SOURCE_DIR}/source/Data/Config DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY ${CMAKE_SOURCE_DIR}/source/Presentation/Input/Keybinds.json DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Set working directory for debugging
set_target_properties(Jello PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

# Copy SFML runtime DLLs next to the executable when SFML is built as shared libs
if(WIN32 AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.21)
    add_custom_command(TARGET Jello POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:Jello> $<TARGET_FILE_DIR:Jello>
        COMMAND_EXPAND_LISTS)
endif()

# Print build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "SFML version: ${SFML_VERSION}")

