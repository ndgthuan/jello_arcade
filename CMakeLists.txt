cmake_minimum_required(VERSION 3.16)
project(JelloGameMenu)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/release)

# Find SFML
find_package(SFML 3.0 REQUIRED COMPONENTS graphics window system audio)

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
)

# Create executable
add_executable(Jello ${SOURCES})

# Link SFML libraries
target_link_libraries(Jello sfml-graphics sfml-window sfml-system sfml-audio)

# Copy assets to output directory
file(COPY ${CMAKE_SOURCE_DIR}/source/Data/Resources/assets DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY ${CMAKE_SOURCE_DIR}/source/Data/Config DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY ${CMAKE_SOURCE_DIR}/source/Presentation/Input/Keybinds.json DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Set working directory for debugging
set_target_properties(Jello PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

# Windows specific settings
if(WIN32)
    # Set output name
    set_target_properties(Jello PROPERTIES OUTPUT_NAME "Jello")
    
    # Copy SFML DLLs if needed (for dynamic linking)
    if(SFML_DYNAMIC)
        # This would copy SFML DLLs, but for static linking we don't need this
    endif()
endif()

# Print build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "SFML version: ${SFML_VERSION}")

