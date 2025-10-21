#pragma once

#include <SFML/Graphics.hpp>
#include "fps-counter.hpp"
#include "particles.hpp"
#include <vector>
#include <cmath>
#include <unordered_map>
#include <functional>

// Custom hash function for grid keys (std::pair<int, int>)
struct KeyHash {
    std::size_t operator()(const Key& k) const {
        // Combine two 32-bit integers into a 64-bit hash
        return (std::size_t(k.first) << 32) | (std::size_t(k.second) & 0xFFFFFFFF);
    }
};

struct Simulation {

    Particles particles;
    sf::CircleShape circle;
    std::unordered_map<Key, std::vector<int>, KeyHash> grid;
    sf::Vector2f system_gravity;
    FPS fps_counter;
    
    float display_x;
    float display_y;
    float delta;

    const int substeps = 3;
    const float force = 1000;
    const float width = 18;

    Simulation(int x, int y, int fps, int argc, char* argv[]) {

        particles.validate(argc, argv);
        particles.reset();

        display_x = x;
        display_y = y;
        delta = 1 / float(fps * substeps);
    }

    void assign_grid() {

        grid.clear();

        // Reserve expected grid size to reduce hash table rehashing
        // Approximate number of cells: (display_x/width) * (display_y/width)
        int expected_cells = (display_x / width) * (display_y / width) * 0.5;  // 50% utilization estimate
        grid.reserve(expected_cells);

        int index = 0;

        for (Particle& i : particles.contents) {
            int x_id = floor(i.position.x / width);
            int y_id = floor(i.position.y / width);

            Key cell_key = {x_id, y_id};
            auto& cell = grid[cell_key];

            // Reserve capacity on first insertion to reduce vector reallocations
            if (cell.empty()) {
                // Estimate ~3 particles per cell on average (can tune based on profiling)
                cell.reserve(4);
            }

            cell.emplace_back(index);
            index += 1;
        }
    }

    void verlet(
        Particle& i,
        Particle& j,
        bool linked,
        bool softbody,
        float adj_target = 0
    ) {

        sf::Vector2f change = i.position - j.position;

        if (change.x == 0 && change.y == 0) {
            return;
        }

        float d_sqrd = change.x * change.x + change.y * change.y;
        float tolerance = i.radius + j.radius;

        if (softbody) {
            tolerance = adj_target;
        }

        // Early exit for non-colliding particles (avoids expensive sqrt)
        // For linked/softbody particles, always resolve constraint
        if (!linked && !softbody && d_sqrd >= tolerance * tolerance) {
            return;
        }

        // Only compute sqrt when we know we need to resolve the constraint
        float distance = sqrt(d_sqrd);
        float i_ratio = i.radius / tolerance;
        float j_ratio = j.radius / tolerance;

        float scalar = 0.5 * (distance - tolerance);
        sf::Vector2f divisor = scalar * change / distance;

        if (!i.fixed || i.fixed_motion) {
            i.position -= divisor * i_ratio;
        }

        if (!j.fixed || j.fixed_motion) {
            j.position += divisor * j_ratio;
        }
    }

    void collide_inner_grid(
        std::vector<int>& inner,
        Key inner_id,
        Key outer_id
    ) {

        bool same_cell = inner_id == outer_id;

        // Use find() to avoid creating empty entries for non-existent cells
        auto outer_it = grid.find(outer_id);
        if (outer_it == grid.end()) {
            return;  // Cell doesn't exist, skip
        }

        std::vector<int>& outer = outer_it->second;
        int start = 0;

        if (inner.size() == 0 || outer.size() == 0) {
            return;
        }

        for (int a = 0; a < inner.size(); a++) {

            if (same_cell) {
                start = a + 1;
            }

            for (int b = start; b < outer.size(); b++) {

                int i_id = inner[a];
                int j_id = outer[b];

                Particle& i = particles.contents[i_id];
                Particle& j = particles.contents[j_id];

                if (i.softbody && j.softbody && (i.body_id == j.body_id)) {
                    continue;
                }

                if (i.linked == j_id || j.linked == i_id) {
                    continue;
                }
                
                verlet(i, j, false, false);
            }
        }
    }

    void collide_grid() {

        for (auto& element : grid) {

            Key id = element.first;
            std::vector<int>& inner = element.second;

            for (int a = id.first - 1; a < (id.first + 1); a++) {
                for (int b = id.second - 1; b < (id.second + 1); b++) {
                    collide_inner_grid(inner, id, {a, b});
                }
            }
        }
    }

    void collide_linked(bool unlink) {

        if (unlink) {
            return;
        }

        for (int index : particles.linked_particles) {
            Particle& i = particles.contents[index];
            Particle& j = particles.contents[i.linked];
            verlet(i, j, true, false);
        }
    }

    void collide_softbody() {
        
        for (auto& softbody : particles.softbodies) {
            for (auto& pairing : softbody.second) {

                Particle& i = particles.contents[pairing.first.first];
                Particle& j = particles.contents[pairing.first.second];
                float dist = pairing.second;
                
                verlet(i, j, false, true, dist);
            }            
        }
    }

    void adjust_gravity(bool zero, bool up, bool left, bool right) {

        system_gravity = {0, force};
        
        if (zero) {
            system_gravity = {0, 0};
        } else if (up) {
            system_gravity = {0, -force};
        } else if (left) {
            system_gravity = {-force, 0};
        } else if (right) {
            system_gravity = {force, 0};
        }
    }
    
    void render(sf::RenderWindow& window, sf::Clock& clock) {

        window.clear();

        for (Particle& i : particles.contents) {
            circle.setPointCount(32);
            circle.setRadius(i.radius * 1.25);
            circle.setPosition(i.position);
            circle.setFillColor(i.color);
            circle.setOrigin({i.radius, i.radius});
            window.draw(circle);
        }

        fps_counter.render(window, clock);
        window.display();
    }
 
    void move(
        sf::RenderWindow& window,
        sf::Vector2i mouse,
        bool center,
        bool explode
    ) {

        for (Particle& i : particles.contents) {

            if (i.is_mouse) {
                i.position.x = mouse.x;
                i.position.y = mouse.y;
            }

            if (i.fixed_motion) {
                i.position = i.motion[i.current];
                i.current = (i.current + 1) % i.motion.size();
            }

            if (i.fixed || i.is_mouse || i.fixed_motion) {
                continue;
            }

            i.position.x = std::clamp(i.position.x, 0.f, display_x - i.radius);
            i.position.y = std::clamp(i.position.y, 0.f, display_y - i.radius);

            sf::Vector2f gravity = system_gravity;
            sf::Vector2f change = i.position - i.previous;
            i.previous = i.position;

            if (center || explode) {
                gravity.x = force * 10 * ((i.position.x / display_x) - 0.5);
                gravity.y = force * 10 * ((i.position.y / display_y) - 0.5);
            }

            if (center) {
                gravity *= -1.0f;
            }

            i.position += change + gravity * delta * delta;
        }
    }

    void advance(
        sf::RenderWindow& window,
        sf::Clock& clock,
        sf::Vector2i mouse,
        bool reset,
        bool up,
        bool left,
        bool right,
        bool zero,
        bool explode,
        bool center,
        bool unlink
    ) {

        if (reset) {
            particles.reset();
        }
               
        for (int i = 0; i < substeps; i++) {
            assign_grid();
            collide_grid();
            collide_softbody();
            collide_linked(unlink);
            move(window, mouse, center, explode);
            adjust_gravity(zero, up, left, right);
        }

        render(window, clock);
    }
};
