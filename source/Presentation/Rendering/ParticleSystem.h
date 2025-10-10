#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <random>
#include <memory>
#include "../UI/UIServices.h"

/**
 * Individual particle structure containing position, velocity, and visual properties
 * Used by the ParticleSystem for creating animated particle effects
 */
struct Particle
{
    sf::Vector2f position; // Current position in world coordinates
    sf::Vector2f velocity; // Movement velocity vector
    float life;            // Current life remaining
    float maxLife;         // Maximum life duration
    float size;            // Visual size of the particle
    sf::Color color;       // Visual color with alpha transparency
};

/**
 * Particle system class that manages animated particle effects
 * Creates, updates, and renders multiple particles with configurable properties
 * Supports particle respawning and boundary wrapping for continuous effects
 */
class ParticleSystem
{
public:
    // Constructor - initializes particle properties and random generator
    ParticleSystem();

    // Initialize particle system with specified number of particles
    void initialize(int particleCount = 400);
    // Initialize with configuration service
    void initialize(std::shared_ptr<IGameConfigurationService> configService);
    // Update all particles based on elapsed time
    void update(float deltaTime);
    // Render all particles to the given window
    void draw(sf::RenderWindow &window);

    // Configuration methods for particle appearance
    void setParticleSize(float minSize, float maxSize);
    void setParticleColor(const sf::Color &color);

private:
    // Helper methods for particle management
    void respawnParticle(Particle &particle); // Reset particle with new random properties
    void limitParticle(Particle &particle);   // Keep particle within screen boundaries

    // Particle storage and generation
    std::vector<Particle> m_particles; // Container for all active particles
    std::mt19937 m_generator;          // Random number generator for particle properties

    // Configurable particle properties
    float m_minSize;           // Minimum particle size
    float m_maxSize;           // Maximum particle size
    sf::Color m_particleColor; // Base color for all particles

    // Configuration service - follows DIP
    std::shared_ptr<IGameConfigurationService> m_configService;
};
