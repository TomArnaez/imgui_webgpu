#pragma once

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.hpp>
#include "imgui_impl_glfw.h"
#include "imgui_impl_wgpu.h"

#include "texture.hpp"

class application {
public:
    application(int width, int height, const char* title);
    ~application();

    void renderFrame();
    GLFWwindow* get_window();

private:
    GLFWwindow* window = nullptr;

    WGPUInstance instance = nullptr;
    WGPUDevice device = nullptr;
    WGPUSurface surface = nullptr;
    WGPUTextureFormat surfaceFormat = WGPUTextureFormat_BGRA8Unorm;

    std::unique_ptr<wgpu_texture> texture_;

    int framebufferWidth;
    int framebufferHeight;
    bool framebufferResized = false;

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    static void glfwErrorCallback(int error, const char* description);
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    bool initGLFW(const char* title);
    bool initWGPU();
    void initImGui();

    void configureSurface();

    void cleanup();
};
