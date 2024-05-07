#pragma once

#include <bitset>
#include <memory>
#include <optional>

#include <vulkan/vulkan.hpp>

#include "kat/utils.hpp"

#include <glm/glm.hpp>

#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>

namespace kat {
    struct window_settings {
        std::string title = "Window";
        glm::uvec2  size  = {600, 800};
    };

    class window {
        explicit window(const window_settings &settings);

      public:
        ~window();

        inline static std::shared_ptr<window> create(const window_settings &settings) { return std::shared_ptr<window>(new window(settings)); }


        [[nodiscard]] bool should_close() const;

        [[nodiscard]] inline vk::SurfaceKHR vulkan_surface() const {
            return m_surface;
        };

      private:
        GLFWwindow *m_window;

        vk::SurfaceKHR m_surface;
    };

    struct queue_support {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> transfer;
        std::optional<uint32_t> present;
        std::optional<uint32_t> compute;
    };

    struct device_queues {
        vk::Queue graphics;
        vk::Queue transfer;
        vk::Queue present;
        vk::Queue compute;
    };

    enum class gpu_selection_strategy { naive };

    class render_device {
        render_device(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface);

      public:
        inline static std::shared_ptr<render_device> create(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface) {
            return std::shared_ptr<render_device>(new render_device(physical_device, surface));
        };

        static std::shared_ptr<render_device> create(vk::PhysicalDevice physical_device, const std::shared_ptr<window> &window);
        static std::shared_ptr<render_device> create(const std::shared_ptr<window> &window, gpu_selection_strategy selection_strategy = gpu_selection_strategy::naive);

        [[nodiscard]] std::string gpu_name() const;

      private:
        vk::PhysicalDevice m_physical_device;
        vk::Device         m_device;

        queue_support m_queue_support;
        device_queues m_device_queues;
    };

    struct global_state {
        global_state();
        ~global_state();

        std::shared_ptr<render_device> m_default_render_device;

        vk::Instance m_instance;

        std::shared_ptr<spdlog::logger> m_logger;
    };

    struct engine_settings {
        std::string    app_name    = "UnnamedApplication";
        utils::version app_version = {.major = 0, .minor = 1, .patch = 0};
    };

    inline global_state *s_global_state = nullptr;

    inline engine_settings s_engine_settings{};

    void set_app_name(const std::string &name);
    void set_app_version(const utils::version &version);

    void init();
    void terminate();

    void                           init_render_device(const std::shared_ptr<window> &window);
    vk::PhysicalDevice             select_gpu(gpu_selection_strategy strategy);
    vk::PhysicalDevice             select_gpu_naive();

    void update_events();

    [[nodiscard]] inline vk::Instance vulkan_instance() {
        return s_global_state->m_instance;
    }

    [[nodiscard]] inline const std::shared_ptr<spdlog::logger>& logger() {
        return s_global_state->m_logger;
    }

    [[nodiscard]] inline const std::shared_ptr<render_device>& default_render_device() {
        return s_global_state->m_default_render_device;
    }
} // namespace kat
