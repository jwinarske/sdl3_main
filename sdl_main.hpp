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

#pragma once

#include <condition_variable>
#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>

#include "asio.hpp"
#include <SDL3/SDL.h>

class SdlMain {
public:
    static SdlMain &getInstance() {
        static SdlMain instance;
        return instance;
    }

    void initialize() {
        running_ = true;

        // Use post instead of co_spawn for synchronous initialization
        asio::post(sdl_strand_, [this]() {
            std::unique_lock lock(init_mutex_);
            std::cout << "SDL initialization starting on thread: " << SDL_GetThreadID(nullptr) << std::endl;

            // SDL3: SDL_InitSubSystem returns true on success
            if (SDL_InitSubSystem(SDL_INIT_VIDEO)) {
                sdl_initialized_ = true;
                custom_event_type_ = SDL_RegisterEvents(1);

                co_spawn(io_context_, [this]() -> asio::awaitable<void> {
                    co_return co_await sdlEventLoop();
                }, asio::detached);

                co_spawn(io_context_, [this]() -> asio::awaitable<void> {
                    co_return co_await periodicTask();
                }, asio::detached);
            } else {
                std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
                sdl_initialized_ = false;
                running_ = false;
            }

            lock.unlock();
            init_cv_.notify_all();
        });

        // Wait for initialization with timeout
        std::unique_lock lock(init_mutex_);
        if (!init_cv_.wait_for(lock, std::chrono::seconds(5), [this] { return sdl_initialized_ || !running_; })) {
            std::cerr << "SDL initialization timed out" << std::endl;
            running_ = false;
        }
    }

    void scheduleTask(std::function<void()> task) {
        co_spawn(io_context_, [this, task = std::move(task)]() -> asio::awaitable<void> {
            co_await dispatch(sdl_strand_, asio::use_awaitable);
            task();
        }, asio::detached);
    }

    void quit() {
        scheduleTask([this]() {
            SDL_Event event;
            event.type = custom_event_type_;
            SDL_PushEvent(&event);
        });
    }

private:
    SdlMain() : io_context_(asio::io_context(ASIO_CONCURRENCY_HINT_1)),
                work_guard_(make_work_guard(io_context_)),
                sdl_strand_(io_context_.get_executor()) {
        thread_ = std::thread([&]() {
            io_context_.run();
        });
    }

    ~SdlMain() {
        thread_.join();
    }

    asio::io_context io_context_;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
    asio::strand<asio::io_context::executor_type> sdl_strand_;

    std::atomic<bool> running_{false};
    bool sdl_initialized_ = false;
    std::thread thread_;
    std::mutex init_mutex_;
    std::condition_variable init_cv_;
    Uint32 custom_event_type_ = 0;

    asio::awaitable<void> sdlEventLoop() {
        SDL_Event event;
        while (running_) {
            while (SDL_PollEvent(&event)) {
                if (event.type == custom_event_type_ || event.type == SDL_EVENT_QUIT) {
                    running_ = false;
                    io_context_.stop();
                    co_return;
                }
                if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                    const auto window = SDL_GetWindowFromID(event.window.windowID);
                    SDL_Log("Closing Window: \"%s\"", SDL_GetWindowTitle(window));
                    SDL_DestroyWindow(window);
                }
            }
            co_await asio::steady_timer(io_context_, std::chrono::milliseconds(10)).async_wait(asio::use_awaitable);
        }
        SDL_Quit();
    }

    asio::awaitable<void> periodicTask() {
        const auto timer = std::make_shared<asio::steady_timer>(io_context_, std::chrono::seconds(2));
        while (running_) {
            co_await timer->async_wait(asio::use_awaitable);
            if (!running_) co_return;
            std::cout << "Periodic SDL strand work: " << SDL_GetCurrentThreadID() << std::endl;
            timer->expires_at(timer->expiry() + std::chrono::seconds(2));
        }
    }
};
