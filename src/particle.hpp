#pragma once
#include <SFML/Graphics.hpp>
#include "hud.hpp"
#include <vector>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

struct Color {
    int r = 000;
    int b = 255;
    int g = 050;
    int a = 150;
    bool ascending = true;
};

struct Particles {

    int n_particle;
    int n_grid_x;
    int n_grid_y;
    int display_x;
    int display_y;
    float width;
    float radius;
    float max_shift;
    float gravity;

    Color color;

    std::vector<float> x_pos;
    std::vector<float> y_pos;
    std::vector<float> x_prv;
    std::vector<float> y_prv;
    std::vector<Color> colors;
    std::vector<float> velocities;

    std::unordered_map<int, std::vector<int>> grid;
    std::unordered_set<int> x_valid;
    std::unordered_set<int> y_valid;

    Button show_state;

    float clamp(float x, float min, float max) {
    
        if (x > max) {
            return max;
        }

        if (x < min) {
            return min;
        }

        return x;
    }

    int id_raw(int x, int y) {
        return x * 73856093 ^ y * 19349663;
    }

    int id_div(float x) {
        return floor(x / width);
    }

    int id(float x, float y) {
        int x_id = id_div(x);
        int y_id = id_div(y);
        return id_raw(x_id, y_id);
    }

    void color_gen() {

        if (color.ascending && color.g < 150) {
            color.g += 1;
        }

        if (color.ascending && color.g == 150) {
            color.ascending = false;
            color.g -= 1;
        }

        if (!color.ascending && color.g > 50) {
            color.g -= 1;
        }

        if (!color.ascending && color.g == 50) {
            color.ascending = true;
            color.g += 1;
        }
    }

    void generate(int n_particle, int display_x) {

        if (x_pos.size() < n_particle) {

            float uniform = float(rand()) / float(RAND_MAX);
            float start_x = float(display_x) * uniform * 0.98;

            x_pos.emplace_back(start_x);
            y_pos.emplace_back(0.5);
            x_prv.emplace_back(start_x);
            y_prv.emplace_back(0);
            colors.emplace_back(color);
            velocities.emplace_back(0);

            color_gen();
        }

        if ((x_pos.size() > n_particle) && (x_pos.size() > 0)) {
            x_pos.pop_back();
            y_pos.pop_back();
            x_prv.pop_back();
            y_prv.pop_back();
            colors.pop_back();
            velocities.pop_back();
        }
    }

    void impose_bounds() {

        for (int i = 0; i < x_pos.size(); i++) {
            x_pos[i] = clamp(x_pos[i], 0, display_x - radius * 2);
            y_pos[i] = clamp(y_pos[i], 0, display_y - radius * 2);
        }
    }

    void assign_grid() {

        grid.clear();
        x_valid.clear();
        y_valid.clear();

        for (int i = 0; i < x_pos.size(); i++) {
            grid[id(x_pos[i], y_pos[i])].emplace_back(i);
            x_valid.insert(id_div(x_pos[i]));
            y_valid.insert(id_div(y_pos[i]));
        }
    }

    void collide_inner(int inner_id, int outer_id) {
        // High performance Verlet integrator which uses
        // a subspace grid to transform search space from
        // O(N^2) to approximately O(N) for particle
        // collision. This detects whether two particles
        // are within some distance and nudges them apart
        // along the axis of collision to solve particle
        // interaction.

        std::vector<int>& inner = grid[inner_id];
        std::vector<int>& outer = grid[outer_id];
        bool same_cell = inner_id == outer_id;

        int start = 0;

        if (inner.size() == 0 || outer.size() == 0) {
            return;
        }

        for (int a = 0; a < inner.size(); a++) {

            if (same_cell) {
                start = a + 1;
            }

            for (int b = start; b < outer.size(); b++) {

                int i = inner[a];
                int j = outer[b];

                float x_chg = x_pos[i] - x_pos[j];
                float y_chg = y_pos[i] - y_pos[j];
                float dist = sqrt(x_chg * x_chg + y_chg * y_chg);

                if ((dist < (radius * 2)) && (dist > 0)) {

                    float delta = 0.5 * (dist - (radius * 2));
                    float x_div = x_chg / dist;
                    float y_div = y_chg / dist;

                    x_pos[i] -= clamp(x_div * delta, -max_shift, max_shift);
                    y_pos[i] -= clamp(y_div * delta, -max_shift, max_shift);
                    x_pos[j] += clamp(x_div * delta, -max_shift, max_shift);
                    y_pos[j] += clamp(y_div * delta, -max_shift, max_shift);
                }
            }
        }
    }

    void collide_grid() {
        // Iterates through the simulation space grid. This
        // cycles through all grid cells and detects collision
        // between the current cell and all neighboring cells.

        for (int i : x_valid) {
            for (int j : y_valid) {
                for (int a = i - 1; a < (i + 1); a++) {
                    for (int b = j - 1; b < (j + 1); b++) {
                        collide_inner(id_raw(i, j), id_raw(a, b));
                    }
                }
            }
        }
    }

    void move(
        float gravity_x,
        float gravity_y,
        float dt,
        bool center,
        bool explode
    ) {

        for (int i = 0; i < x_pos.size(); i++) {

            float x_chg = x_pos[i] - x_prv[i];
            float y_chg = y_pos[i] - y_prv[i];

            x_prv[i] = x_pos[i];
            y_prv[i] = y_pos[i];

            float grv_x = gravity_x;
            float grv_y = gravity_y;

            if (center || explode) {
                grv_x = gravity * 10 * ((x_pos[i] / display_x) - 0.5);
                grv_y = gravity * 10 * ((y_pos[i] / display_y) - 0.5);
            }

            if (center) {
                grv_x *= -1;
                grv_y *= -1;
            }

            x_pos[i] += x_chg + grv_x * dt * dt;
            y_pos[i] += y_chg + grv_y * dt * dt;

            velocities[i] = pow(x_chg, 2) + pow(y_chg, 2);
        }
    }

    void render(sf::RenderWindow& window, bool press) {

        sf::CircleShape circle(radius * 1.75);
        circle.setPointCount(32);
        show_state.toggle(press);

        for (int i = 0; i < x_pos.size(); i++) {

            float x = x_pos[i];
            float y = y_pos[i];
            circle.setPosition(x, y);

            sf::Color particle_color(
                colors[i].r,
                colors[i].g,
                colors[i].b,
                colors[i].a
            );

            if (show_state.state) {
                int velocity = int(clamp(velocities[i], 0, 5) * 50);
                particle_color.r = velocity;
                particle_color.g = 255 - velocity;
                particle_color.b = 0;
            } 

            circle.setFillColor(particle_color);
            window.draw(circle);
        }
    }
};
