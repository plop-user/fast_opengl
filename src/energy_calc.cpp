

#include <SDL2/SDL_image.h>
#include <glad/glad.h>
#include <glslread.h>

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <phy.h>




double calculateTotalEnergy(std::vector<body> bodies) {
    double kineticEnergy = 0.0;
    double potentialEnergy = 0.0;
    const double G = 1.0; // Must match your updateGravity G

    for (size_t i = 0; i < bodies.size(); ++i) {
        // 1. Kinetic Energy: 0.5 * m * v^2
        double vSq = glm::dot(bodies[i].velocity, bodies[i].velocity);
        kineticEnergy += 0.5 * bodies[i].mass * vSq;

        // 2. Potential Energy: Pairwise gravity -G * m1 * m2 / r
        for (size_t j = i + 1; j < bodies.size(); ++j) {
            glm::dvec3 diff = bodies[j].pos - bodies[i].pos;
            double dist = std::sqrt(glm::dot(diff, diff) + 0.0); // Use same softening
            
            potentialEnergy -= (G * bodies[i].mass * bodies[j].mass) / dist;
        }
    }

    return kineticEnergy + potentialEnergy;
}
