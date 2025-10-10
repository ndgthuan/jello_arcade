#include "Presentation/Core/GameWindow.h"
#include "Presentation/UI/UIServices.h"
#include "Data/Logging/Logger.h"

int main()
{
    // Initialize logger
    Logger::getInstance().initialize("game.log");
    Logger::getInstance().info("Starting Jello Game Application");

    try
    {
        // Create and initialize the main game window
        GameWindow window;

        // Inject resource service into game window
        auto resourceServicePtr = std::make_shared<ResourceService>();
        Logger::getInstance().info("Loading resource configuration...");
        if (!resourceServicePtr->loadConfig("source/Data/Config/resource_config.json"))
        {
            Logger::getInstance().error("Failed to load resource configuration");
            return -1;
        }
        Logger::getInstance().info("Resource configuration loaded successfully");
        window.setResourceService(resourceServicePtr);

        Logger::getInstance().info("Game window created successfully");

        // Main game loop - process events and render until window is closed
        while (window.isOpen())
        {
            window.processEvents();
            window.display();
        }

        Logger::getInstance().info("Game application closed normally");
    }
    catch (const std::exception &e)
    {
        Logger::getInstance().critical("Fatal error in main: " + std::string(e.what()));
        return -1;
    }

    return 0;
}
