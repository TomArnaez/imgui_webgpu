// application.cpp
#include "application.hpp"
#include "glfw3webgpu.h"
#include <stdexcept>
#include <cstdio>

application::application(int width, int height, const char* title)
    : framebufferWidth(width), framebufferHeight(height)
{
    if (!initGLFW(title))
        throw std::runtime_error("Failed to initialize GLFW");
    if (!initWGPU())
        throw std::runtime_error("Failed to initialize WGPU");

    std::vector<uint8_t> vec(512*512*4, 122);
    std::dextents<uint32_t, 2> exts(512, 512);
    WGPUQueue queue = wgpuDeviceGetQueue(device);

    texture_ = std::make_unique<wgpu_texture>(device, queue, exts, reinterpret_cast<std::byte*>(vec.data()));

    configureSurface();
    initImGui();
}

application::~application() {
    cleanup();
}

void application::glfwErrorCallback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void application::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<application*>(glfwGetWindowUserPointer(window));
    app->framebufferWidth = width;
    app->framebufferHeight = height;
    app->framebufferResized = true;
}

bool application::initGLFW(const char* title) {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit())
        return false;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(framebufferWidth, framebufferHeight, title, nullptr, nullptr);
    if (!window)
        return false;

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    return true;
}

bool application::initWGPU() {
    instance = wgpuCreateInstance(nullptr);
    if (!instance)
        return false;

    WGPUAdapter adapter = nullptr;
    wgpuInstanceRequestAdapter(instance, nullptr,
        [](WGPURequestAdapterStatus status, WGPUAdapter adapter, const char* message, void* userdata) {
            if (status == WGPURequestAdapterStatus_Success)
                *(WGPUAdapter*)userdata = adapter;
            else
                fprintf(stderr, "Adapter error: ");
        }, &adapter);
    if (!adapter)
        return false;

    wgpuAdapterRequestDevice(adapter, nullptr,
        [](WGPURequestDeviceStatus status, WGPUDevice device, const char* message, void* userdata) {
            if (status == WGPURequestDeviceStatus_Success)
                *(WGPUDevice*)userdata = device;
            else
                fprintf(stderr, "Device error");
        }, &device);
    if (!device)
        return false;

    surface = glfwGetWGPUSurface(instance, window);
    if (!surface)
        return false;

    wgpuDeviceSetUncapturedErrorCallback(device,
        [](WGPUErrorType type, const char* message, void*) {
            fprintf(stderr, "WGPU Error");
        }, nullptr);

    return true;
}

void application::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;

    ImGui_ImplGlfw_InitForOther(window, true);

    ImGui_ImplWGPU_InitInfo init_info{};
    init_info.Device = device;
    init_info.RenderTargetFormat = surfaceFormat;
    init_info.NumFramesInFlight = 3;
    ImGui_ImplWGPU_Init(&init_info);
}

void application::configureSurface() {
    WGPUSurfaceConfiguration config = {};
    config.device = device;
    config.format = surfaceFormat;
    config.usage = WGPUTextureUsage_RenderAttachment;
    config.width = framebufferWidth;
    config.height = framebufferHeight;
    config.presentMode = WGPUPresentMode_Fifo;

    wgpuSurfaceConfigure(surface, &config);
}

void application::renderFrame() {
    if (framebufferResized) {
        framebufferResized = false;
        wgpuSurfaceUnconfigure(surface);
        configureSurface();
        return;
    }

    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();

    {
        ImGui::Begin("WebGPU Texture Test");

        ImGui::Image((ImTextureID)(intptr_t)texture_->get_view(), ImVec2(512, 512));

        ImGui::End();
    }

    ImGui::Render();

    WGPUSurfaceTexture surfaceTexture{};
    wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);
    WGPUTextureView view = wgpuTextureCreateView(surfaceTexture.texture, nullptr);

    WGPURenderPassColorAttachment color_attachments = {};
    color_attachments.loadOp = WGPULoadOp_Clear;
    color_attachments.storeOp = WGPUStoreOp_Store;
    color_attachments.clearValue = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
    color_attachments.view = view;

    WGPURenderPassDescriptor render_pass_desc{};
    render_pass_desc.colorAttachmentCount = 1;
    render_pass_desc.colorAttachments = &color_attachments;
    render_pass_desc.depthStencilAttachment = nullptr;

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
    WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_desc);
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);
    wgpuRenderPassEncoderEnd(pass);

    WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuQueueSubmit(wgpuDeviceGetQueue(device), 1, &commandBuffer);

    wgpuSurfacePresent(surface);

    wgpuTextureViewRelease(view);
    wgpuRenderPassEncoderRelease(pass);
    wgpuCommandEncoderRelease(encoder);
    wgpuCommandBufferRelease(commandBuffer);
}

GLFWwindow* application::get_window() {
    return window;
}

void application::cleanup() {
    wgpuSurfaceUnconfigure(surface);
    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(glfwGetCurrentContext());
    glfwTerminate();
}

// Main entry point
int main() {
    try {
        application app(1280, 720, "ImGui WebGPU Example");
        GLFWwindow* window = app.get_window();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            app.renderFrame();
        }
    } catch (const std::exception& e) {
        fprintf(stderr, "Exception: %s\n", e.what());
        return -1;
    }
    return 0;
}
