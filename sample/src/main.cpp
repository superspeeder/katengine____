#include <iostream>
#include <kat/engine.hpp>

void run() {
    auto window = kat::window::create(kat::window_settings {
        .title = "Hello!",
        .size = {800, 600},
    });

    kat::init_render_device(window);

    while (!window->should_close()) {
        kat::update_events();

    }
}

int main() {
    kat::init();

    try {
        run();
    } catch (std::exception& e) {
        SPDLOG_CRITICAL("Fatal Error: {}", e.what());
    }

    kat::terminate();
    return 0;
}
