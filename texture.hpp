#pragma once
#include <webgpu/webgpu.hpp>
#include <mdspan>

class wgpu_texture {
    WGPUTexture texture_handle_;
    WGPUTextureView texture_view_;
    std::dextents<uint32_t, 2> extents_;

public:
    wgpu_texture(WGPUDevice device, WGPUQueue queue, std::dextents<uint32_t, 2> extents, const std::byte* data)
        : extents_(extents) {
        WGPUTextureDescriptor texture_desc;
        texture_desc.nextInChain = NULL;
        texture_desc.label = NULL;
        texture_desc.usage = WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding;
        texture_desc.dimension = WGPUTextureDimension_2D;
        texture_desc.size.width = extents.extent(1);
        texture_desc.size.height = extents.extent(0);
        texture_desc.size.depthOrArrayLayers = 1;
        texture_desc.format = WGPUTextureFormat_RGBA8Unorm;
        texture_desc.mipLevelCount = 1;
        texture_desc.sampleCount = 1;
        texture_desc.viewFormatCount = 0;
        texture_desc.viewFormats = NULL;
    
        WGPUTexture texture_handle = wgpuDeviceCreateTexture(device, &texture_desc);
    
        if (!texture_handle)
            throw std::runtime_error("Failed to create wgpu texture");
    
        texture_handle_ = texture_handle;

        WGPUImageCopyTexture dest;
        dest.nextInChain = NULL;
        dest.texture = texture_handle_;
        dest.mipLevel = 0;
        dest.origin.x = 0;
        dest.origin.y = 0;
        dest.origin.z = 0;
        dest.aspect = WGPUTextureAspect_All;

        WGPUTextureDataLayout src;
        src.nextInChain = NULL;
        src.offset = 0;
        src.bytesPerRow = 4 * extents.extent(1);
        src.rowsPerImage = extents.extent(0);

        const size_t bytes = src.bytesPerRow * src.rowsPerImage;
        WGPUExtent3D write_size;
        write_size.width = extents.extent(1);
        write_size.height = extents.extent(0);
        write_size.depthOrArrayLayers = 1;
        wgpuQueueWriteTexture(queue, &dest, data, bytes, &src, &write_size);
    
        WGPUTextureViewDescriptor view_desc;
        view_desc.nextInChain = NULL;
        view_desc.label = NULL;
        view_desc.format = WGPUTextureFormat_RGBA8Unorm;
        view_desc.dimension = WGPUTextureViewDimension_2D;
        view_desc.baseMipLevel = 0;
        view_desc.mipLevelCount = 1;
        view_desc.baseArrayLayer = 0;
        view_desc.arrayLayerCount = 1;
        view_desc.aspect = WGPUTextureAspect_All;
    
        texture_view_ = wgpuTextureCreateView(texture_handle_, &view_desc);

        if (!texture_view_)
            throw std::runtime_error("Failed to create wgpu texture view");
    }

    WGPUTextureView get_view() const {
        return texture_view_;
    }

    ~wgpu_texture() {
    }
};