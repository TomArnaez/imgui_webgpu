#pragma once

#include <webgpu/webgpu.hpp>

struct wgpu_helper {
    WGPUInstance instance = nullptr;
    WGPUDevice device = nullptr;
    WGPUQueue queue = nullptr;

    wgpu_helper() {
        instance = wgpuCreateInstance(nullptr);

        WGPUAdapter adapter = nullptr;
        wgpuInstanceRequestAdapter(instance, nullptr,
            [](WGPURequestAdapterStatus status, WGPUAdapter adapter, const char* message, void* userdata) {
                if (status == WGPURequestAdapterStatus_Success)
                    *(WGPUAdapter*)userdata = adapter;
                else
                    fprintf(stderr, "Adapter error: ");
            }, &adapter);
    
        wgpuAdapterRequestDevice(adapter, nullptr,
            [](WGPURequestDeviceStatus status, WGPUDevice device, const char* message, void* userdata) {
                if (status == WGPURequestDeviceStatus_Success)
                    *(WGPUDevice*)userdata = device;
                else
                    fprintf(stderr, "Device error");
            }, &device);
            
        queue = wgpuDeviceGetQueue(device);
    }
};