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
        int gravity_x,
        int gravity_y,
        int particle,
        int particle_max,
        int grid_x,
        int grid_y,
        bool press,
        bool is_center,
        bool is_explode
    ) {

        show_state.toggle(press);

        int fps = 1 / clock.getElapsedTime().asSeconds();
        clock.restart();

        if (show_state.state) {

            std::string gx_ch = std::to_string(gravity_x);
            std::string gy_ch = std::to_string(gravity_y);
            std::string grv_msg = "(" + gx_ch + ", " + gy_ch + ")";

            if (is_center || is_explode) {
                grv_msg = "Dynamic";
            }

            text.setString(
                "FPS: " + std::to_string(fps) + "\n" +
                "Particles: " + std::to_string(particle) +
                " of " + std::to_string(particle_max) + "\n" +
                "Grid Cells: " + std::to_string(grid_x) +
                " by " + std::to_string(grid_y) + "\n" +
                "Gravity: " + grv_msg + "\n\n" +
                "Control Gravity: WASDZXC\n" +
                "Add Particles: E\n" +
                "Remove Particles: Q\n" +
                "Toggle Velocity View: V"
            );

            window.draw(text);
        }
    }
};
