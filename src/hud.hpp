#pragma once
#include <SFML/Graphics.hpp>
#include <string>

struct Button {

    bool previous = false;
    bool state = false;

    void toggle(bool press = false) {
        if (press && !previous) {
            previous = true;
            state = !state;
        } else if (press) {
            previous = true;
        } else {
            previous = false;
        }
    }
};

struct HUD {

    int font_size = 16;
    int font_offset = 10;
    bool previous = false;
    bool state = false;

    std::string font_path = "../font/jetbrains.ttf";
    sf::Color font_color = sf::Color::White;
    
    sf::Font font;
    sf::Text text;

    Button show_state;

    HUD() {
        font.loadFromFile(font_path);
        text.setFont(font);
        text.setCharacterSize(font_size);
        text.setPosition(font_offset, font_offset);
        text.setFillColor(font_color);
    }

    void render(
        sf::RenderWindow& window,
        sf::Clock& clock,
        int gravity_x = 0,
        int gravity_y = 0,
        int current_particle = 0,
        int n_particle = 0,
        int n_grid_x = 0,
        int n_grid_y = 0,
        bool press = false,
        bool center = false,
        bool explode = false
    ) {

        show_state.toggle(press);

        int fps = 1 / clock.getElapsedTime().asSeconds();
        clock.restart();

        if (show_state.state) {

            std::string fps_ch = std::to_string(fps);
            std::string curr_chr = std::to_string(current_particle);
            std::string part_ch = std::to_string(n_particle);
            std::string grid_ch_x = std::to_string(n_grid_x);
            std::string grid_ch_y = std::to_string(n_grid_y);
            std::string grid_ch = std::to_string(n_grid_x * n_grid_y);
            std::string gx_ch = std::to_string(gravity_x);
            std::string gy_ch = std::to_string(gravity_y);
            std::string wasd = "Gravity: W, A, S, D, Z\n";
            std::string add = "Add Particles: E\n";
            std::string remove = "Remove Particles: Q\n";
            std::string velocity = "Toggle Velocities: V\n";
            std::string dyn_cntr = "Center/Explode: C, X\n";

            if (center || explode) {
                gx_ch = "Dynamic";
                gy_ch = "Dynamic";
            }

            text.setString(
                "FPS: " + fps_ch + "\n" +
                "Particles: " + curr_chr + " of " + part_ch + "\n" +
                "Grid Cells: " + grid_ch + " = " + grid_ch_x + " x " +
                grid_ch_y + "\n" + "Gravity: (" + gx_ch + ", " + gy_ch +
                ")\n\n" + wasd + add + remove + velocity + dyn_cntr
            );

            window.draw(text);
        }
    }
};
