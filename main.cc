// @copyright Copyright (c) 2025 Joel Winarske
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <iostream>

#include "sdl_main.hpp"

SDL_Window *window[3];

int main() {
    SdlMain &sdl_main = SdlMain::getInstance();

    sdl_main.initialize();

    sdl_main.scheduleTask([&]() {
        window[0] = SDL_CreateWindow("Asio Strand with SDL3 1", 640, 480, SDL_WINDOW_RESIZABLE);
        if (!window[0]) {
            std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
            sdl_main.quit();
        }
    });

    sdl_main.scheduleTask([&]() {
        window[1] = SDL_CreateWindow("Asio Strand with SDL3 2", 640, 480, SDL_WINDOW_RESIZABLE);
        if (!window[1]) {
            std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
            sdl_main.quit();
        }
    });

    sdl_main.scheduleTask([&]() {
        window[2] = SDL_CreateWindow("Asio Strand with SDL3 3", 640, 480, SDL_WINDOW_RESIZABLE);
        if (!window[2]) {
            std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
            sdl_main.quit();
        }
    });
}
