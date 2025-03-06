#include "wgpu_helper.hpp"
#include <iostream>
#include <fstream>
#include <vector>

int main() {
    wgpu_helper helper;

    std::ifstream file("C:\\dev\\repos\\imgui_webgpu\\shaders\\prefix_scan.spv", std::ios::binary);
    if (!file)
        throw std::runtime_error("Failed to open file!");

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Create a vector to hold the bytes.
    std::vector<char> buffer(size);

    if (file.read(buffer.data(), size))
        std::cout << "Read " << buffer.size() << " bytes from file." << std::endl;
    else
        throw std::runtime_error("Failed to reads file!");

    WGPUShaderModuleSPIRVDescriptor spirvDesc = {};
    spirvDesc.chain.sType = WGPUSType_ShaderModuleSPIRVDescriptor;
    spirvDesc.codeSize = size / 4;
    spirvDesc.code = reinterpret_cast<uint32_t*>(buffer.data());

    WGPUShaderModuleDescriptor shaderDesc = {};
    shaderDesc.nextInChain = &spirvDesc.chain;
    WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(helper.device, &shaderDesc);

    constexpr size_t BUFFER_SIZE = 1024 * sizeof(int);

    WGPUBufferDescriptor inputBufferDesc = {};
    inputBufferDesc.label = "Input Buffer";
    inputBufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc | WGPUBufferUsage_Storage;
    inputBufferDesc.size = BUFFER_SIZE;
    inputBufferDesc.mappedAtCreation = true;

    WGPUBuffer inputBuffer = wgpuDeviceCreateBuffer(helper.device, &inputBufferDesc);

    int* data = (int*)wgpuBufferGetMappedRange(inputBuffer, 0, BUFFER_SIZE);
    size_t elementCount = BUFFER_SIZE / sizeof(int);
    for (size_t i = 0; i < elementCount; ++i)
        data[i] = 1;
    wgpuBufferUnmap(inputBuffer);
    
    // Staging buffer for data download
    WGPUBufferDescriptor stagingBufferDesc = {};
    stagingBufferDesc.label = "Staging Buffer";
    stagingBufferDesc.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
    stagingBufferDesc.size = BUFFER_SIZE;
    stagingBufferDesc.mappedAtCreation = false;

    WGPUBuffer stagingBuffer = wgpuDeviceCreateBuffer(helper.device, &stagingBufferDesc);

    WGPUBindGroupLayoutEntry bindGroupLayoutEntries[2] = {
        { // Input buffer
            .binding = 0,
            .visibility = WGPUShaderStage_Compute,
            .buffer = {
                .type = WGPUBufferBindingType_Storage
            }
        }
    };
    
    WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {
        .entryCount = 1,
        .entries = bindGroupLayoutEntries
    };
    WGPUBindGroupLayout bindGroupLayout = wgpuDeviceCreateBindGroupLayout(helper.device, &bindGroupLayoutDesc);
    
    // Pipeline Layout
    WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &bindGroupLayout
    };
    WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(helper.device, &pipelineLayoutDesc);
    
    // Compute Pipeline
    WGPUComputePipelineDescriptor computePipelineDesc = {
        .layout = pipelineLayout,
        .compute = {
            .module = shaderModule,
            .entryPoint = "main"
        }
    };
    WGPUComputePipeline computePipeline = wgpuDeviceCreateComputePipeline(helper.device, &computePipelineDesc);

    WGPUBindGroupEntry bindGroupEntries[1] = {
        {
            .binding = 0,
            .buffer = inputBuffer,
            .offset = 0,
            .size = BUFFER_SIZE
        }
    };
    
    WGPUBindGroupDescriptor bindGroupDesc = {
        .layout = bindGroupLayout,
        .entryCount = 1,
        .entries = bindGroupEntries
    };
    WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(helper.device, &bindGroupDesc);

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(helper.device, nullptr);

    // Compute pass
    WGPUComputePassDescriptor computePassDesc = {};
    WGPUComputePassEncoder computePass = wgpuCommandEncoderBeginComputePass(encoder, &computePassDesc);
    wgpuComputePassEncoderSetPipeline(computePass, computePipeline);
    wgpuComputePassEncoderSetBindGroup(computePass, 0, bindGroup, 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroups(computePass, 16, 1, 1); // Adjust based on shader
    wgpuComputePassEncoderEnd(computePass);

    // Copy results to staging buffer
    wgpuCommandEncoderCopyBufferToBuffer(encoder, inputBuffer, 0, stagingBuffer, 0, BUFFER_SIZE);

    // Finish and submit
    WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuQueueSubmit(helper.queue, 1, &commandBuffer);

    wgpuBufferMapAsync(stagingBuffer, WGPUMapMode_Read, 0, BUFFER_SIZE, 
        [](WGPUBufferMapAsyncStatus status, void* userdata) {
            std::cout << "Mapped!" << std::endl;

            if (status == WGPUBufferMapAsyncStatus_Success) {
                auto staging_buffer = reinterpret_cast<WGPUBuffer*>(userdata);
                const int* data = static_cast<int*>(wgpuBufferGetMappedRange(*staging_buffer, 0, BUFFER_SIZE));

                std::cout << data[63] << std::endl;
                wgpuBufferUnmap(*staging_buffer);
            }
        }, &stagingBuffer);

    while (true) {
        wgpuDevicePoll(helper.device, false, nullptr);
    }
}