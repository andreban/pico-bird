/**
 * Copyright 2021 Andre Cipriani Bandarra
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pico/stdlib.h"
#include "pico_display.hpp"
#include "bitmap.h"
#include "sprites.h"
#include <string>

const uint8_t BIRD_SIZE = 16;
const uint8_t BOARD_WIDTH = pimoroni::PicoDisplay::WIDTH;
const uint8_t BOARD_HEIGHT = pimoroni::PicoDisplay::HEIGHT;

uint16_t buffer[pimoroni::PicoDisplay::WIDTH * pimoroni::PicoDisplay::HEIGHT];
pimoroni::PicoDisplay pico_display(buffer);

static unsigned int g_seed;


/**
 * // Used to seed the generator.
 */
inline void fast_srand(int seed) {
    g_seed = seed;
}

/**
 *  Compute a pseudorandom integer.
 *  Output value in range [0, 32767]
 *  See https://stackoverflow.com/a/26237777/1249994
 */
inline int fast_rand(void) {
    g_seed = (214013 * g_seed + 2531011);
    return (g_seed >> 16) & 0x7FFF;
}

/**
 * The game state.
 */
enum GameState {
    MENU,
    GAME_PLAY,
    GAME_OVER
};

/**
 * The Player.
 */
class Player {
public:
    pimoroni::Rect bounds = pimoroni::Rect(32, 65, BIRD_SIZE, BIRD_SIZE);
    Bitmap *bitmap[9] = {
            new Bitmap(16, 16, Sprites::PLAYER_BITMAP),
            new Bitmap(16, 16, Sprites::PLAYER_BITMAP),
            new Bitmap(16, 16, Sprites::PLAYER_BITMAP),
            new Bitmap(16, 16, Sprites::PLAYER_BITMAP2),
            new Bitmap(16, 16, Sprites::PLAYER_BITMAP2),
            new Bitmap(16, 16, Sprites::PLAYER_BITMAP2),
            new Bitmap(16, 16, Sprites::PLAYER_BITMAP3),
            new Bitmap(16, 16, Sprites::PLAYER_BITMAP3),
            new Bitmap(16, 16, Sprites::PLAYER_BITMAP3),
    };
    int current_image = 0;

    Player() {
    }

    void reset() {
        bounds.y = 65;
    }

    void update() {
        int speed = pico_display.is_pressed(pimoroni::PicoDisplay::Y) ? -1 : 1;
        bounds.y += speed;
        current_image++;
    }

    void render() {
        bitmap[current_image % 9]->draw(&pico_display, this->bounds.x, this->bounds.y);
    }
};

/**
 * The obstacle walls.
 */
class Obstacle {
public:
    pimoroni::Rect top_wall;
    pimoroni::Rect bottom_wall;
    Bitmap *bottom_image = new Bitmap(32, 135, Sprites::PIPE_BOTTOM);
    Bitmap *top_image = new Bitmap(32, 135, Sprites::PIPE_TOP);
    int start_x;

    Obstacle(int start_x) {
        this->start_x = start_x;
        reset();
    }

    void reset() {
        int rand = fast_rand();
        uint8_t y_gap = (rand % 64) + 32;
        uint8_t top_wall_height = (rand % (BOARD_HEIGHT - y_gap)) + 1;
        uint8_t bottom_wall_height = BOARD_HEIGHT - top_wall_height - y_gap;
        top_wall = pimoroni::Rect(start_x, 0, 32, top_wall_height);
        bottom_wall = pimoroni::Rect(
                start_x, BOARD_HEIGHT - bottom_wall_height, 32, bottom_wall_height);
    }

    void update() {
        top_wall.x -= 1;
        bottom_wall.x -= 1;
        if (top_wall.x + top_wall.w < 0 || bottom_wall.x + bottom_wall.w < 0) {
            top_wall.x = BOARD_WIDTH;
            bottom_wall.x = BOARD_WIDTH;
        }
    }

    void render() {
        top_image->draw_reverse(&pico_display, top_wall.x, top_wall.y + top_wall.h);
        bottom_image->draw(&pico_display, bottom_wall.x, bottom_wall.y);
    }
};

GameState gameState = GameState::MENU;

const int NUM_OBSTACLES = 3;
Obstacle *obstacles[NUM_OBSTACLES] = {
        new Obstacle(BOARD_WIDTH),
        new Obstacle(BOARD_WIDTH + BOARD_WIDTH / 3),
        new Obstacle(BOARD_WIDTH + (BOARD_WIDTH / 3 * 2))
};
Player *player = new Player();
uint32_t score = 0;

void resetGame() {
    player->reset();
    for (int i = 0; i < NUM_OBSTACLES; i++) {
        obstacles[i]->reset();
    }
    score = 0;
}

/**
 * Renders the game state.
 */
void gameplay() {
    // Clear screen
    pico_display.set_pen(34, 3, 77);
    pico_display.clear();

    player->update();
    for (int i = 0; i < NUM_OBSTACLES; i++) {
        obstacles[i]->update();
    }

    if (player->bounds.y < 0 ||
        player->bounds.y >= BOARD_HEIGHT - 15) {
        gameState = GameState::GAME_OVER;
    }

    for (int i = 0; i < NUM_OBSTACLES; i++) {
        if (player->bounds.intersects(obstacles[i]->top_wall) ||
            player->bounds.intersects(obstacles[i]->bottom_wall)) {
            gameState = GameState::GAME_OVER;
            break;
        }
    }

    // render game
    for (int i = 0; i < NUM_OBSTACLES; i++) {
        obstacles[i]->render();
    }
    player->render();
    score++;
    pico_display.set_pen(221, 221, 221);
    pico_display.text("Score: " + std::to_string(score), pimoroni::Point(180, 0), 0);
}

/**
 * Renders the menu state.
 */
void menu() {
    pico_display.set_pen(221, 221, 221);
    pico_display.text("Pico Bird", pimoroni::Point(8, 15), 240, 5);
    pico_display.text("Press Any Button", pimoroni::Point(35, 70), 240, 2);
    pico_display.text("Sprites by TheWitch7971", pimoroni::Point(5, 115), 200, 1);
    pico_display.text("Code by andreban", pimoroni::Point(5, 125), 200, 1);
    if (pico_display.is_pressed(pimoroni::PicoDisplay::A) ||
        pico_display.is_pressed(pimoroni::PicoDisplay::B) ||
        pico_display.is_pressed(pimoroni::PicoDisplay::X) ||
        pico_display.is_pressed(pimoroni::PicoDisplay::Y)) {
        resetGame();
        gameState = GameState::GAME_PLAY;
    }
}

/**
 * Renders the gameover state.
 */
void gameover() {
    pico_display.set_pen(221, 221, 221);
    pico_display.text("Game Over", pimoroni::Point(100, 55), 0);
    if (pico_display.is_pressed(pimoroni::PicoDisplay::A) ||
        pico_display.is_pressed(pimoroni::PicoDisplay::B) ||
        pico_display.is_pressed(pimoroni::PicoDisplay::X) ||
        pico_display.is_pressed(pimoroni::PicoDisplay::Y)) {
        resetGame();
        gameState = GameState::GAME_PLAY;
    }
}

int main() {
    pico_display.init();
    pico_display.set_backlight(255);
    pico_display.set_pen(34, 3, 77);
    pico_display.clear();
    while (true) {
        switch (gameState) {
            case GameState::MENU:
                menu();
                break;
            case GameState::GAME_PLAY:
                gameplay();
                break;
            case GameState::GAME_OVER:
                gameover();
                break;
        }
        pico_display.update();
    }
}


