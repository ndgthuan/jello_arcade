#include "ParticleSystem.h"

ParticleSystem::ParticleSystem() : m_minSize(8.0f), m_maxSize(15.0f), m_particleColor(255, 255, 255, 50)
{
    // Initialize random number generator for particle properties
    std::random_device rd;
    m_generator.seed(rd());
}

void ParticleSystem::initialize(int particleCount)
{
    // Clear existing particles and reserve memory
    m_particles.clear();
    m_particles.reserve(particleCount);

    // Define random distributions for particle properties
    std::uniform_real_distribution<float> posX(-2000.0f, 6000.0f);
    std::uniform_real_distribution<float> posY(-500.0f, 1500.0f);
    std::uniform_real_distribution<float> velX(-20.0f, 20.0f);
    std::uniform_real_distribution<float> velY(-15.0f, 15.0f);
    std::uniform_real_distribution<float> life(3.0f, 8.0f);
    std::uniform_real_distribution<float> size(m_minSize, m_maxSize);

    // Create particles with random properties
    for (int i = 0; i < particleCount; ++i)
    {
        Particle p;
        p.position = sf::Vector2f(posX(m_generator), posY(m_generator));
        p.velocity = sf::Vector2f(velX(m_generator), velY(m_generator));
        p.maxLife = life(m_generator);
        p.life = p.maxLife;
        p.size = size(m_generator);
        p.color = m_particleColor;

        m_particles.push_back(p);
    }
}

void ParticleSystem::update(float deltaTime)
{
    // Update each particle's position and life
    for (auto &particle : m_particles)
    {
        // Update position based on velocity and time
        particle.position += particle.velocity * deltaTime;

        // Decrease life time
        particle.life -= deltaTime;

        // Respawn particle if it dies
        if (particle.life <= 0.0f)
        {
            respawnParticle(particle);
        }

        // Keep particles within bounds
        limitParticle(particle);
    }
}

void ParticleSystem::draw(sf::RenderWindow &window)
{
    // Draw each particle as a circle
    for (const auto &particle : m_particles)
    {
        sf::CircleShape circle(particle.size);
        circle.setFillColor(particle.color);
        // Center the circle on particle position
        circle.setPosition(particle.position - sf::Vector2f(particle.size, particle.size));

        window.draw(circle);
    }
}

void ParticleSystem::setParticleSize(float minSize, float maxSize)
{
    // Update particle size range
    m_minSize = minSize;
    m_maxSize = maxSize;
}

void ParticleSystem::setParticleColor(const sf::Color &color)
{
    // Update particle color
    m_particleColor = color;
}

void ParticleSystem::respawnParticle(Particle &particle)
{
    // Define random distributions for respawning
    std::uniform_real_distribution<float> posX(-2000.0f, 6000.0f);
    std::uniform_real_distribution<float> posY(-500.0f, 1500.0f);
    std::uniform_real_distribution<float> velX(-20.0f, 20.0f);
    std::uniform_real_distribution<float> velY(-15.0f, 15.0f);
    std::uniform_real_distribution<float> life(3.0f, 8.0f);
    std::uniform_real_distribution<float> size(m_minSize, m_maxSize);

    // Assign new random properties to the particle
    particle.position = sf::Vector2f(posX(m_generator), posY(m_generator));
    particle.velocity = sf::Vector2f(velX(m_generator), velY(m_generator));
    particle.life = life(m_generator);
    particle.maxLife = particle.life;
    particle.size = size(m_generator);
}

void ParticleSystem::limitParticle(Particle &particle)
{
    // Wrap particles around the screen boundaries
    if (particle.position.x < -2000.0f)
        particle.position.x = 6000.0f;
    if (particle.position.x > 6000.0f)
        particle.position.x = -2000.0f;
    if (particle.position.y < -500.0f)
        particle.position.y = 1500.0f;
    if (particle.position.y > 1500.0f)
        particle.position.y = -500.0f;
}

void ParticleSystem::initialize(std::shared_ptr<IGameConfigurationService> configService)
{
    // Store configuration service
    m_configService = configService;

    // Initialize with default particle count from configuration
    int particleCount = m_configService ? m_configService->getDefaultParticleCount() : 400;
    initialize(particleCount);
}
