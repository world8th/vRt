#pragma once

#include <stdio.h>
#include <vulkan/vulkan.hpp> // only for inner usage
#include <vulkan/vk_mem_alloc.h>

// include STL
#include <chrono>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <array>
#include <map>
#include <random>
#include <vector>
#include <algorithm>
#include <iterator>
#include <cstddef>
#include <optional>

#ifdef VRT_ENABLE_EXECUTION_POLICY
#include <execution>
#define VRT_ASYNC(F) std::async(std::launch::async|std::launch::deferred,F);
#else
#define VRT_ASYNC(F) std::async(F);
#endif



namespace _vt {
    constexpr auto DEFAULT_FENCE_TIMEOUT = 100000000000ll;
    constexpr auto INTENSIVITY = 4096ull;
    constexpr auto ATTRIB_EXTENT = 8ull; // no way to set more than it now

    template <typename T>
    inline auto sgn(T val) { return (T(0) < val) - (val < T(0)); }

    template<class T = uint32_t>
    inline T tiled(T sz, T gmaxtile) {
        // return (int32_t)ceil((double)sz / (double)gmaxtile);
        return sz <= 0 ? 0 : (sz / gmaxtile + sgn(sz % gmaxtile));
    }

    inline double milliseconds() {
        auto duration = std::chrono::high_resolution_clock::now();
        double millis = std::chrono::duration_cast<std::chrono::nanoseconds>(
            duration.time_since_epoch())
            .count() /
            1000000.0;
        return millis;
    }

    template <class T>
    inline size_t strided(size_t sizeo) { return sizeof(T) * sizeo; }


    // read binary (for SPIR-V)
    inline auto readBinary(std::string filePath) {
        std::ifstream file(filePath, std::ios::in | std::ios::binary | std::ios::ate);
        std::vector<uint32_t> data;
        if (file.is_open()) {
            std::streampos size = file.tellg();
            data.resize(tiled(size_t(size), sizeof(uint32_t)));
            file.seekg(0, std::ios::beg);
            file.read((char *)data.data(), size);
            file.close();
        } else {
            std::cerr << "Failure to open " + filePath << std::endl;
        }
        return data;
    };

    // read source (unused)
    inline std::string readSource(const std::string &filePath, const bool &lineDirective = false) {
        std::string content = "";
        std::ifstream fileStream(filePath, std::ios::in);
        if (!fileStream.is_open()) {
            std::cerr << "Could not read file " << filePath << ". File does not exist."
                << std::endl;
            return "";
        }
        std::string line = "";
        while (!fileStream.eof()) {
            std::getline(fileStream, line);
            if (lineDirective || line.find("#line") == std::string::npos)
                content.append(line + "\n");
        }
        fileStream.close();
        return content;
    };


    // general command buffer barrier
    inline void commandBarrier(const VkCommandBuffer& cmdBuffer) {
        VkMemoryBarrier memoryBarrier;
        memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        memoryBarrier.pNext = nullptr;
        memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(
            cmdBuffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            {}, //VK_DEPENDENCY_BY_REGION_BIT,
            1, &memoryBarrier,
            0, nullptr,
            0, nullptr);
    };

    // from host command buffer barrier
    inline void fromHostCommandBarrier(const VkCommandBuffer& cmdBuffer) {
        VkMemoryBarrier memoryBarrier;
        memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        memoryBarrier.pNext = nullptr;
        memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_HOST_READ_BIT;
        memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(
            cmdBuffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_HOST_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            {}, //VK_DEPENDENCY_BY_REGION_BIT,
            1, &memoryBarrier,
            0, nullptr,
            0, nullptr);
    };

    // to host command buffer barrier
    inline void toHostCommandBarrier(const VkCommandBuffer& cmdBuffer) {
        VkMemoryBarrier memoryBarrier;
        memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        memoryBarrier.pNext = nullptr;
        memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_HOST_READ_BIT;

        vkCmdPipelineBarrier(
            cmdBuffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_HOST_BIT,
            {}, //VK_DEPENDENCY_BY_REGION_BIT,
            1, &memoryBarrier,
            0, nullptr,
            0, nullptr);
    };

    inline void updateCommandBarrier(const VkCommandBuffer& cmdBuffer) {
        commandBarrier(cmdBuffer);
    };

    // create secondary command buffers for batching compute invocations
    inline auto createCommandBuffer(const VkDevice device, const VkCommandPool cmdPool, bool secondary = true, bool once = true) {
        VkCommandBuffer cmdBuffer = nullptr;

        VkCommandBufferAllocateInfo cmdi;
        cmdi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdi.pNext = nullptr;
        cmdi.commandPool = cmdPool;
        cmdi.level = secondary ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdi.commandBufferCount = 1;
        vkAllocateCommandBuffers(device, &cmdi, &cmdBuffer);

        VkCommandBufferInheritanceInfo inhi;
        inhi.pNext = nullptr;
        inhi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inhi.pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;

        VkCommandBufferBeginInfo bgi;
        bgi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bgi.pNext = nullptr;
        bgi.flags = {};
        bgi.flags = once ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        bgi.pInheritanceInfo = secondary ? &inhi : nullptr;
        vkBeginCommandBuffer(cmdBuffer, &bgi);

        return cmdBuffer;
    };


    inline auto loadAndCreateShaderModuleInfo(const std::vector<uint32_t>& code) {
        VkShaderModuleCreateInfo smi;
        smi.pNext = nullptr;
        smi.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        smi.pCode = (uint32_t *)code.data();
        smi.codeSize = code.size()*4;
        smi.flags = {};
        return smi;
    }

    // create shader module
    inline auto loadAndCreateShaderModule(VkDevice device, const std::vector<uint32_t>& code) {
        VkShaderModule sm = nullptr;
        vkCreateShaderModule(device, &loadAndCreateShaderModuleInfo(code), nullptr, &sm);
        return sm;
    };

    // create shader module
    inline auto loadAndCreateShaderModuleStage(VkDevice device, const std::vector<uint32_t>& code, const char * entry = "main") {
        VkPipelineShaderStageCreateInfo spi;
        spi.pNext = nullptr;
        spi.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        spi.flags = {};
        spi.module = loadAndCreateShaderModule(device, code);
        spi.pName = entry;
        spi.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        spi.pSpecializationInfo = nullptr;
        return spi;
    };

    // create compute pipelines
    inline auto createCompute(VkDevice device, std::string path, VkPipelineLayout layout, VkPipelineCache cache) {
        auto code = readBinary(path);
        auto spi = loadAndCreateShaderModuleStage(device, code);

        VkComputePipelineCreateInfo cmpi;
        cmpi.pNext = nullptr;
        cmpi.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        cmpi.flags = {};
        cmpi.layout = layout;
        cmpi.stage = spi;
        cmpi.basePipelineHandle = {};
        cmpi.basePipelineIndex = -1;

        VkPipeline pipeline = nullptr;
        vkCreateComputePipelines(device, cache, 1, &cmpi, nullptr, &pipeline);
        return pipeline;
    }

    // create compute pipelines
    inline auto createCompute(VkDevice device, const VkPipelineShaderStageCreateInfo& spi, VkPipelineLayout layout, VkPipelineCache cache) {
        VkComputePipelineCreateInfo cmpi;
        cmpi.pNext = nullptr;
        cmpi.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        cmpi.flags = {};
        cmpi.layout = layout;
        cmpi.stage = spi;
        cmpi.basePipelineHandle = {};
        cmpi.basePipelineIndex = -1;

        VkPipeline pipeline = nullptr;
        vkCreateComputePipelines(device, cache, 1, &cmpi, nullptr, &pipeline);
        return pipeline;
    }


    // create compute pipelines
    inline auto createComputeMemory(VkDevice device, const std::vector<uint32_t>& code, VkPipelineLayout layout, VkPipelineCache cache) {
        auto spi = loadAndCreateShaderModuleStage(device, code);

        VkComputePipelineCreateInfo cmpi;
        cmpi.pNext = nullptr;
        cmpi.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        cmpi.flags = {};
        cmpi.layout = layout;
        cmpi.stage = spi;
        cmpi.basePipelineHandle = {};
        cmpi.basePipelineIndex = -1;

        VkPipeline pipeline = nullptr;
        vkCreateComputePipelines(device, cache, 1, &cmpi, nullptr, &pipeline);
        return pipeline;
    }

    // add dispatch in command buffer (with default pipeline barrier)
    inline VkResult cmdDispatch(VkCommandBuffer cmd, VkPipeline pipeline, uint32_t x = 1, uint32_t y = 1, uint32_t z = 1, bool barrier = true) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdDispatch(cmd, x, y, z);
        if (barrier) {
            commandBarrier(cmd); // put shader barrier
        }
        return VK_SUCCESS;
    }

    // low level copy command between (prefer for host and device)
    inline VkResult cmdCopyBufferL(VkCommandBuffer cmd, vk::Buffer srcBuffer, vk::Buffer dstBuffer, const std::vector<vk::BufferCopy>& regions, const std::function<void(const VkCommandBuffer&)>& barrierFn = commandBarrier) {
        vk::CommandBuffer(cmd).copyBuffer(srcBuffer, dstBuffer, regions);
        barrierFn(cmd); // put copy barrier
        return VK_SUCCESS;
    }

    // short data set with command buffer (alike push constant)
    template<class T>
    inline VkResult cmdUpdateBuffer(VkCommandBuffer cmd, const std::vector<T>& data, vk::Buffer dstBuffer, VkDeviceSize offset = 0) {
        vk::CommandBuffer(cmd).updateBuffer(dstBuffer, offset, data);
        //updateCommandBarrier(cmd);
        return VK_SUCCESS;
    }

    // template function for fill buffer by constant value
    // use for create repeat variant
    template<uint32_t Rv>
    inline VkResult cmdFillBuffer(VkCommandBuffer cmd, VkBuffer dstBuffer, VkDeviceSize size = VK_WHOLE_SIZE, intptr_t offset = 0) {
        vk::CommandBuffer(cmd).fillBuffer(vk::Buffer(dstBuffer), offset, size, Rv);
        //updateCommandBarrier(cmd);
        return VK_SUCCESS;
    }

    // make whole size buffer descriptor info
    inline auto bufferDescriptorInfo(vk::Buffer buffer, vk::DeviceSize offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE) {
        return vk::DescriptorBufferInfo(buffer, offset, size);
    }

    // submit command (with async wait)
    inline void submitCmd(VkDevice device, VkQueue queue, std::vector<VkCommandBuffer> cmds, vk::SubmitInfo smbi = {}) {
        // no commands 
        if (cmds.size() <= 0) return;

        smbi.commandBufferCount = cmds.size();
        smbi.pCommandBuffers = (vk::CommandBuffer *)cmds.data();

        VkFence fence = nullptr; VkFenceCreateInfo fin{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr };
        vkCreateFence(device, &fin, nullptr, &fence);
        vkQueueSubmit(queue, 1, (const VkSubmitInfo *)&smbi, fence);
        vkWaitForFences(device, 1, &fence, true, DEFAULT_FENCE_TIMEOUT);
        vkDestroyFence(device, fence, nullptr);
    };

    // once submit command buffer
    inline void submitOnce(VkDevice device, VkQueue queue, VkCommandPool cmdPool, std::function<void(const VkCommandBuffer&)> cmdFn = {}, vk::SubmitInfo smbi = {}) {
        auto cmdBuf = createCommandBuffer(device, cmdPool, false); cmdFn(cmdBuf); vkEndCommandBuffer(cmdBuf);
        submitCmd(device, queue, { cmdBuf });
        vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuf); // free that command buffer
    };

    // submit command (with async wait)
    inline void submitCmdAsync(VkDevice device, VkQueue queue, std::vector<VkCommandBuffer> cmds, std::function<void()> asyncCallback = {}, vk::SubmitInfo smbi = {}) {
        // no commands 
        if (cmds.size() <= 0) return;

        smbi.commandBufferCount = cmds.size();
        smbi.pCommandBuffers = (const vk::CommandBuffer *)cmds.data();

        VkFence fence = nullptr; VkFenceCreateInfo fin{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr };
        vkCreateFence(device, &fin, nullptr, &fence);
        vkQueueSubmit(queue, 1, (const VkSubmitInfo *)&smbi, fence);
        VRT_ASYNC([=]() {
            vkWaitForFences(device, 1, &fence, true, DEFAULT_FENCE_TIMEOUT);
            VRT_ASYNC([=]() {
                vkDestroyFence(device, fence, nullptr);
                if (asyncCallback) asyncCallback();
            });
        });
    };

    // once submit command buffer
    inline void submitOnceAsync(VkDevice device, VkQueue queue, VkCommandPool cmdPool, std::function<void(VkCommandBuffer)> cmdFn = {}, std::function<void(const VkCommandBuffer&)> asyncCallback = {}, vk::SubmitInfo smbi = {}) {
        auto cmdBuf = createCommandBuffer(device, cmdPool, false); cmdFn(cmdBuf); vkEndCommandBuffer(cmdBuf);
        submitCmdAsync(device, queue, { cmdBuf }, [=]() {
            asyncCallback(cmdBuf); // call async callback
            vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuf); // free that command buffer
        });
    };

    template <class T>
    inline auto makeVector(const T*ptr, size_t size = 1) {
        std::vector<T>v(size); memcpy(v.data(), ptr, strided<T>(size));
        return v;
    }

    // create fence function
    inline vk::Fence createFence(VkDevice device, bool signaled = true) {
        vk::FenceCreateInfo info;
        if (signaled) info.setFlags(vk::FenceCreateFlagBits::eSignaled);
        return vk::Device(device).createFence(info);
    }

    // List of all possible required extensions
    // Raytracing itself should support extension filtering and NVidia GPU alongside of RX Vega
    // Can work in single GPU systems (AMD or NVidia)
    inline static std::vector<const char *> raytracingRequiredExtensions = {
        "VK_AMD_gpu_shader_int16",
        "VK_AMD_gpu_shader_half_float",
        "VK_AMD_buffer_marker",
        "VK_AMD_shader_info",
        "VK_AMD_shader_ballot",
        "VK_AMD_texture_gather_bias_lod",
        "VK_AMD_shader_image_load_store_lod",
        "VK_AMD_gcn_shader",
        "VK_AMD_shader_trinary_minmax",
        "VK_KHR_8bit_storage",
        "VK_KHR_16bit_storage",
        "VK_KHR_descriptor_update_template",
        "VK_KHR_push_descriptor",
        "VK_KHR_image_format_list",
        "VK_KHR_sampler_mirror_clamp_to_edge",
        "VK_KHR_storage_buffer_storage_class",
        "VK_KHR_variable_pointers",
        "VK_KHR_relaxed_block_layout",
        "VK_KHR_get_memory_requirements2",
        "VK_KHR_get_physical_device_properties2",
        "VK_KHR_get_surface_capabilities2",
        "VK_KHR_bind_memory2",
        "VK_KHR_maintenance1",
        "VK_KHR_maintenance2",
        "VK_KHR_maintenance3"
    };
};
