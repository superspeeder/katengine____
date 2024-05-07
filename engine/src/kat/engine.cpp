#include "engine.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <unordered_set>

namespace kat {

    global_state::global_state() {

        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

        spdlog::sinks_init_list sinks = {sink};

        m_logger = std::make_shared<spdlog::logger>("katengine", sinks);


        glfwInit();

        SPDLOG_LOGGER_INFO(m_logger, "Initialized GLFW");

        vk::ApplicationInfo app_info =
            vk::ApplicationInfo(s_engine_settings.app_name.c_str(),
                                VK_MAKE_API_VERSION(0, s_engine_settings.app_version.major, s_engine_settings.app_version.minor, s_engine_settings.app_version.patch), "KatEngine",
                                VK_MAKE_API_VERSION(0, 0, 1, 0), VK_API_VERSION_1_3);

        uint32_t     count;
        const char **extensions = glfwGetRequiredInstanceExtensions(&count);

        m_instance = vk::createInstance(vk::InstanceCreateInfo({}, &app_info, 0, nullptr, count, extensions));
        SPDLOG_LOGGER_INFO(m_logger, "Created Vulkan Instance");
    }

    global_state::~global_state() = default;

    window::window(const window_settings &settings) {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_RESIZABLE, false);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        m_window = glfwCreateWindow(static_cast<int>(settings.size.x), static_cast<int>(settings.size.y), settings.title.c_str(), nullptr, nullptr);
        SPDLOG_LOGGER_INFO(logger(), "Created Window");

        VkSurfaceKHR surf;
        glfwCreateWindowSurface(vulkan_instance(), m_window, nullptr, &surf);
        m_surface = surf;
    }

    window::~window() {
        glfwDestroyWindow(m_window);
    }

    bool window::should_close() const {
        return glfwWindowShouldClose(m_window);
    }

    render_device::render_device(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface) : m_physical_device(physical_device) {
        // get queue support
        auto     queue_families = m_physical_device.getQueueFamilyProperties();
        uint32_t i              = 0;

        bool graphics_selected = false;
        bool present_selected  = false;

        for (const auto &family : queue_families) {
            // pick the first graphics family we see (normally a good idea, on nvidia the 0th family is the "best" and most versatile).
            if (!graphics_selected && family.queueFlags & vk::QueueFlagBits::eGraphics) {
                graphics_selected        = true;
                m_queue_support.graphics = i;

                // if possible, make present family the same as graphics.
                if (m_physical_device.getSurfaceSupportKHR(i, surface)) {
                    m_queue_support.present = i;
                    present_selected        = true;
                }
            }

            // try to select a transfer family that isn't the same as the graphics one.
            if (graphics_selected && i != m_queue_support.graphics && family.queueFlags & vk::QueueFlagBits::eTransfer) {
                m_queue_support.transfer = i;
            }

            // backup to pick a present family if we don't get one from graphics.
            if (!present_selected && m_physical_device.getSurfaceSupportKHR(i, surface)) {
                m_queue_support.present = i;
                present_selected        = true;
            }

            if (family.queueFlags & vk::QueueFlagBits::eCompute) {
                m_queue_support.compute = i;
            }

            if (m_queue_support.graphics.has_value() && m_queue_support.present.has_value() && m_queue_support.compute.has_value() && m_queue_support.transfer.has_value()) {
                break;
            }
            i++;
        }

        // graphics family always supports transfer ops, so we can fallback to just using that family if we don't find another transfer family.
        if (!m_queue_support.transfer.has_value()) {
            m_queue_support.transfer = m_queue_support.graphics.value();
        }

        SPDLOG_LOGGER_INFO(logger(), "Picked Queues (graphics: {}, present: {}, transfer: {}, compute: {})", m_queue_support.graphics.value(), m_queue_support.present.value(),
                           m_queue_support.transfer.value(), m_queue_support.compute.value());


        std::unordered_set<uint32_t> unique_families = {m_queue_support.graphics.value(), m_queue_support.present.value(), m_queue_support.transfer.value(),
                                                        m_queue_support.compute.value()};

        std::vector<vk::DeviceQueueCreateInfo> device_queue_create_infos;
        device_queue_create_infos.reserve(unique_families.size());

        std::array<float, 1> queue_priorities = {1.0f};

        for (const auto &family : unique_families) {
            device_queue_create_infos.emplace_back(vk::DeviceQueueCreateFlags(), family, queue_priorities);
        }

        std::vector<const char *> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

        vk::PhysicalDeviceFeatures2 pdf2{};
        pdf2.features.fillModeNonSolid   = true;
        pdf2.features.geometryShader     = true;
        pdf2.features.tessellationShader = true;
        pdf2.features.wideLines          = true;
        pdf2.features.largePoints        = true;

        vk::PhysicalDeviceVulkan11Features v11f{};
        v11f.variablePointers              = true;
        v11f.variablePointersStorageBuffer = true;

        vk::PhysicalDeviceVulkan12Features v12f{};
        v12f.imagelessFramebuffer        = true;
        v12f.timelineSemaphore           = true;
        v12f.uniformBufferStandardLayout = true;

        vk::PhysicalDeviceVulkan13Features v13f{};
        v13f.dynamicRendering   = true;
        v13f.inlineUniformBlock = true;

        pdf2.pNext = &v11f;
        v11f.pNext = &v12f;
        v12f.pNext = &v13f;

        m_device = m_physical_device.createDevice(vk::DeviceCreateInfo(vk::DeviceCreateFlags{}, device_queue_create_infos, {}, extensions, nullptr, &pdf2));

        m_device_queues.graphics = m_device.getQueue(m_queue_support.graphics.value(), 0);
        m_device_queues.present = m_device.getQueue(m_queue_support.present.value(), 0);
        m_device_queues.transfer = m_device.getQueue(m_queue_support.transfer.value(), 0);
        m_device_queues.compute = m_device.getQueue(m_queue_support.compute.value(), 0);

        SPDLOG_LOGGER_INFO(logger(), "Created Device");
    }

    std::shared_ptr<render_device> render_device::create(vk::PhysicalDevice physical_device, const std::shared_ptr<window> &window) {
        return std::shared_ptr<render_device>(new render_device(physical_device, window->vulkan_surface()));
    }

    std::shared_ptr<render_device> render_device::create(const std::shared_ptr<window> &window, gpu_selection_strategy selection_strategy) {
        vk::PhysicalDevice gpu = select_gpu(selection_strategy);
        return std::shared_ptr<render_device>(new render_device(gpu, window->vulkan_surface()));
    }

    std::string render_device::gpu_name() const {
        vk::PhysicalDeviceProperties props = m_physical_device.getProperties();
        return props.deviceName;
    }

    void set_app_name(const std::string &name) {
        s_engine_settings.app_name = name;
    }

    void set_app_version(const utils::version &version) {
        s_engine_settings.app_version = version;
    }

    void init() {
        s_global_state = new global_state();
    }

    void terminate() {
        delete s_global_state;
    }

    void init_render_device(const std::shared_ptr<window> &window) {
        s_global_state->m_default_render_device = render_device::create(window, gpu_selection_strategy::naive);
        SPDLOG_LOGGER_INFO(logger(), "Initialized Render Device");
        SPDLOG_LOGGER_INFO(logger(), "Selected GPU: {}", default_render_device()->gpu_name());
    }

    vk::PhysicalDevice select_gpu(gpu_selection_strategy strategy) {
        switch (strategy) {
        case gpu_selection_strategy::naive:
            return select_gpu_naive();
        default:
            throw std::runtime_error("Unrecognized or unimplemented gpu_selection_strategy.");
        }
    }

    vk::PhysicalDevice select_gpu_naive() {
        auto gpus = vulkan_instance().enumeratePhysicalDevices();
        return gpus[0];
    }

    void update_events() {
        glfwPollEvents();
    }
} // namespace kat
