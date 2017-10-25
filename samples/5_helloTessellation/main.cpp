#include <stdexcept>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "window.h"
#include "vulkan_instance.h"
#include "vulkan_shader_module.h"
#include "vulkan_buffer.h"
#include "camera.h"
#include <iostream>

VulkanInstance* instance;
VulkanDevice* device;
VulkanSwapChain* swapchain;
Camera* camera;
glm::mat4* mappedCameraView;

namespace {
    bool buttons[GLFW_MOUSE_BUTTON_LAST + 1] = { 0 };

    void mouseButtonCallback(GLFWwindow*, int button, int action, int) {
        buttons[button] = (action == GLFW_PRESS);
    }

    void cursorPosCallback(GLFWwindow*, double mouseX, double mouseY) {
        static double oldX, oldY;
        float dX = static_cast<float>(mouseX - oldX);
        float dY = static_cast<float>(mouseY - oldY);
        oldX = mouseX;
        oldY = mouseY;

        if (buttons[2] || (buttons[0] && buttons[1])) {
            camera->pan(-dX * 0.002f, dY * -0.002f);
            memcpy(mappedCameraView, &camera->view(), sizeof(glm::mat4));
        }
        else if (buttons[0]) {
            camera->rotate(dX * -0.01f, dY * -0.01f);
            memcpy(mappedCameraView, &camera->view(), sizeof(glm::mat4));
        }
        else if (buttons[1]) {
            camera->zoom(dY * -0.005f);
            memcpy(mappedCameraView, &camera->view(), sizeof(glm::mat4));
        }
    }

    void scrollCallback(GLFWwindow*, double, double yoffset) {
        camera->zoom(static_cast<float>(yoffset) * 0.04f);
        memcpy(mappedCameraView, &camera->view(), sizeof(glm::mat4));
    }
}

struct Vertex {
    glm::vec4 position;
    glm::vec4 color;
};

struct CameraUBO {
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
};

struct ModelUBO {
    glm::mat4 modelMatrix;
};

VkRenderPass CreateRenderPass() {
    // Color buffer attachment represented by one of the images from the swap chain
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapchain->GetImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Create an attachment reference to be used with subpass
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Create subpass description
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    // Specify subpass dependency
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // Create render pass
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkRenderPass renderPass;
    if (vkCreateRenderPass(device->GetVulkanDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass");
    }

    return renderPass;
}

VkDescriptorSetLayout CreateDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding> layoutBindings) {
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pNext = nullptr;
    descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    descriptorSetLayoutCreateInfo.pBindings = layoutBindings.data();

    VkDescriptorSetLayout descriptorSetLayout;
    vkCreateDescriptorSetLayout(device->GetVulkanDevice(), &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout);
    return descriptorSetLayout;
}

VkDescriptorPool CreateDescriptorPool() {
    // Info for the types of descriptors that can be allocated from this pool
    VkDescriptorPoolSize poolSizes[3];
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount = 1;

    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.pNext = nullptr;
    descriptorPoolInfo.poolSizeCount = 3;
    descriptorPoolInfo.pPoolSizes = poolSizes;
    descriptorPoolInfo.maxSets = 3;

    VkDescriptorPool descriptorPool;
    vkCreateDescriptorPool(device->GetVulkanDevice(), &descriptorPoolInfo, nullptr, &descriptorPool);
    return descriptorPool;
}

VkDescriptorSet CreateDescriptorSet(VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout) {
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    VkDescriptorSet descriptorSet;
    vkAllocateDescriptorSets(device->GetVulkanDevice(), &allocInfo, &descriptorSet);
    return descriptorSet;
}

VkPipelineLayout CreatePipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayouts) {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = 0;

    VkPipelineLayout pipelineLayout;
    if (vkCreatePipelineLayout(device->GetVulkanDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    return pipelineLayout;
}

VkPipeline CreateGraphicsPipeline(VkPipelineLayout pipelineLayout, VkRenderPass renderPass, unsigned int subpass) {
    VkShaderModule vertShaderModule = createShaderModule("5_helloTessellation/shaders/shader.vert.spv", device->GetVulkanDevice());
	VkShaderModule tescShaderModule = createShaderModule("5_helloTessellation/shaders/shader.tesc.spv", device->GetVulkanDevice());
	VkShaderModule teseShaderModule = createShaderModule("5_helloTessellation/shaders/shader.tese.spv", device->GetVulkanDevice());
    VkShaderModule fragShaderModule = createShaderModule("5_helloTessellation/shaders/shader.frag.spv", device->GetVulkanDevice());

    // Assign each shader module to the appropriate stage in the pipeline
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo tescShaderStageInfo = {};
	tescShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	tescShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	tescShaderStageInfo.module = tescShaderModule;
	tescShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo teseShaderStageInfo = {};
	teseShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	teseShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	teseShaderStageInfo.module = teseShaderModule;
	teseShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, tescShaderStageInfo, teseShaderStageInfo, fragShaderStageInfo };

    // --- Set up fixed-function stages ---

    // Vertex input binding
    VkVertexInputBindingDescription vertexInputBinding = {};
    vertexInputBinding.binding = 0;
    vertexInputBinding.stride = sizeof(Vertex);
    vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // Inpute attribute bindings describe shader attribute locations and memory layouts
    std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributes;
    // Attribute location 0: Position
    vertexInputAttributes[0].binding = 0;
    vertexInputAttributes[0].location = 0;
    // Position attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
    vertexInputAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributes[0].offset = offsetof(Vertex, position);
    // Attribute location 1: Color
    vertexInputAttributes[1].binding = 0;
    vertexInputAttributes[1].location = 1;
    // Color attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
    vertexInputAttributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributes[1].offset = offsetof(Vertex, color);

    // Vertex input
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexInputBinding;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributes.data();

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Tessellation state
	VkPipelineTessellationStateCreateInfo tessellationInfo = {};
	tessellationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	tessellationInfo.pNext = NULL;
	tessellationInfo.flags = 0;
	tessellationInfo.patchControlPoints = 1;

    // Viewports and Scissors (rectangles that define in which regions pixels are stored)
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchain->GetExtent().width);
    viewport.height = static_cast<float>(swapchain->GetExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = swapchain->GetExtent();

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    // Multisampling (turned off here)
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    // Color blending (turned off here, but showing options for learning)
    // --> Configuration per attached framebuffer
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    // --> Global color blending settings
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    // --- Create graphics pipeline ---
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 4;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pTessellationState = &tessellationInfo;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = subpass;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(device->GetVulkanDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline");
    }

    // No need for the shader modules anymore
    vkDestroyShaderModule(device->GetVulkanDevice(), vertShaderModule, nullptr);
	vkDestroyShaderModule(device->GetVulkanDevice(), tescShaderModule, nullptr);
	vkDestroyShaderModule(device->GetVulkanDevice(), teseShaderModule, nullptr);
    vkDestroyShaderModule(device->GetVulkanDevice(), fragShaderModule, nullptr);

    return pipeline;
}

VkPipeline CreateComputePipeline(VkPipelineLayout pipelineLayout) {
    VkShaderModule compShaderModule = createShaderModule("5_helloTessellation/shaders/shader.comp.spv", device->GetVulkanDevice());

    VkPipelineShaderStageCreateInfo compShaderStageInfo = {};
    compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    compShaderStageInfo.module = compShaderModule;
    compShaderStageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = compShaderStageInfo;
    pipelineInfo.layout = pipelineLayout;

    VkPipeline pipeline;
    if (vkCreateComputePipelines(device->GetVulkanDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline");
    }
    
    vkDestroyShaderModule(device->GetVulkanDevice(), compShaderModule, nullptr);

    return pipeline;
}

std::vector<VkFramebuffer> CreateFrameBuffers(VkRenderPass renderPass) {
    std::vector<VkFramebuffer> frameBuffers(swapchain->GetCount());
    for (uint32_t i = 0; i < swapchain->GetCount(); i++) {
        VkImageView attachments[] = { swapchain->GetImageView(i) };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchain->GetExtent().width;
        framebufferInfo.height = swapchain->GetExtent().height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device->GetVulkanDevice(), &framebufferInfo, nullptr, &frameBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer");
        }
    }
    return frameBuffers;
}

int main(int argc, char** argv) {
    static constexpr char* applicationName = "Hello Tessellation";
    InitializeWindow(640, 480, applicationName);

    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    instance = new VulkanInstance(applicationName, glfwExtensionCount, glfwExtensions);

    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance->GetVkInstance(), GetGLFWWindow(), nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface");
    }

    instance->PickPhysicalDevice({ VK_KHR_SWAPCHAIN_EXTENSION_NAME }, QueueFlagBit::GraphicsBit | QueueFlagBit::TransferBit | QueueFlagBit::ComputeBit | QueueFlagBit::PresentBit, surface);

	// --- Specify the set of device features used ---
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.tessellationShader = VK_TRUE;
	deviceFeatures.fillModeNonSolid = VK_TRUE;

    device = instance->CreateDevice(QueueFlagBit::GraphicsBit | QueueFlagBit::TransferBit | QueueFlagBit::ComputeBit | QueueFlagBit::PresentBit, deviceFeatures);
    swapchain = device->CreateSwapChain(surface);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = instance->GetQueueFamilyIndices()[QueueFlags::Graphics];
    poolInfo.flags = 0;

    VkCommandPool commandPool;
    if (vkCreateCommandPool(device->GetVulkanDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }

    CameraUBO cameraTransforms;
    camera = new Camera(&cameraTransforms.viewMatrix);
    cameraTransforms.projectionMatrix = glm::perspective(static_cast<float>(45 * M_PI / 180), 640.f / 480.f, 0.1f, 1000.f);

    ModelUBO modelTransforms;
	modelTransforms.modelMatrix = glm::mat4(1.0);//glm::rotate(glm::mat4(1.f), static_cast<float>(15 * M_PI / 180), glm::vec3(0.f, 0.f, 1.f));

    std::vector<Vertex> vertices = {
        { { 0.0f,  1.0f, 0.0f, 1.f },{ 0.0f, 1.0f, 0.0f, 1.f } },
    };

    unsigned int vertexBufferSize = static_cast<uint32_t>(vertices.size() * sizeof(vertices[0]));

    // Create vertex and index buffers
    VkBuffer vertexBuffer = CreateBuffer(device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, vertexBufferSize);
    unsigned int vertexBufferOffsets[1];
    VkDeviceMemory vertexBufferMemory = AllocateMemoryForBuffers(device, { vertexBuffer }, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBufferOffsets);

    // Copy data to buffer memory
    {
        char* data;
        vkMapMemory(device->GetVulkanDevice(), vertexBufferMemory, 0, vertexBufferSize, 0, reinterpret_cast<void**>(&data));
        memcpy(data + vertexBufferOffsets[0], vertices.data(), vertexBufferSize);
        vkUnmapMemory(device->GetVulkanDevice(), vertexBufferMemory);
    }

    // Bind the memory to the buffers
    BindMemoryForBuffers(device, vertexBufferMemory, { vertexBuffer }, vertexBufferOffsets);

    // Create uniform buffers
    VkBuffer cameraBuffer = CreateBuffer(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(CameraUBO));
    VkBuffer modelBuffer = CreateBuffer(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(ModelUBO));
    unsigned int uniformBufferOffsets[2];
    VkDeviceMemory uniformBufferMemory = AllocateMemoryForBuffers(device, { cameraBuffer, modelBuffer }, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBufferOffsets);

    // Copy data to uniform memory
    {
        char* data;
        vkMapMemory(device->GetVulkanDevice(), uniformBufferMemory, 0, uniformBufferOffsets[1] + sizeof(ModelUBO), 0, reinterpret_cast<void**>(&data));
        memcpy(data + uniformBufferOffsets[0], &cameraTransforms, sizeof(CameraUBO));
        memcpy(data + uniformBufferOffsets[1], &modelTransforms, sizeof(ModelUBO));
        vkUnmapMemory(device->GetVulkanDevice(), uniformBufferMemory);
    }

    // Bind the memory to the buffers
    BindMemoryForBuffers(device, uniformBufferMemory, { cameraBuffer, modelBuffer }, uniformBufferOffsets);

    VkDescriptorPool descriptorPool = CreateDescriptorPool();

    VkDescriptorSetLayout computeSetLayout = CreateDescriptorSetLayout({
        { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
    });

    VkDescriptorSetLayout cameraSetLayout = CreateDescriptorSetLayout({
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT , nullptr },
    });

    VkDescriptorSetLayout modelSetLayout = CreateDescriptorSetLayout({
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr },
    });

    VkDescriptorSet computeSet = CreateDescriptorSet(descriptorPool, computeSetLayout);
    VkDescriptorSet cameraSet = CreateDescriptorSet(descriptorPool, cameraSetLayout);
    VkDescriptorSet modelSet = CreateDescriptorSet(descriptorPool, modelSetLayout);

    // Initialize descriptor sets
    {
        VkDescriptorBufferInfo computeBufferInfo = {};
        computeBufferInfo.buffer = vertexBuffer;
        computeBufferInfo.offset = 0;
        computeBufferInfo.range = vertexBufferSize;

        VkWriteDescriptorSet writeComputeInfo = {};
        writeComputeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeComputeInfo.dstSet = computeSet;
        writeComputeInfo.dstBinding = 0;
        writeComputeInfo.descriptorCount = 1;
        writeComputeInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeComputeInfo.pBufferInfo = &computeBufferInfo;

        VkDescriptorBufferInfo cameraBufferInfo = {};
        cameraBufferInfo.buffer = cameraBuffer;
        cameraBufferInfo.offset = 0;
        cameraBufferInfo.range = sizeof(CameraUBO);

        VkWriteDescriptorSet writeCameraInfo = {};
        writeCameraInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeCameraInfo.dstSet = cameraSet;
        writeCameraInfo.dstBinding = 0;
        writeCameraInfo.descriptorCount = 1;
        writeCameraInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeCameraInfo.pBufferInfo = &cameraBufferInfo;

        VkDescriptorBufferInfo modelBufferInfo = {};
        modelBufferInfo.buffer = modelBuffer;
        modelBufferInfo.offset = 0;
        modelBufferInfo.range = sizeof(ModelUBO);

        VkWriteDescriptorSet writeModelInfo = {};
        writeModelInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeModelInfo.dstSet = modelSet;
        writeModelInfo.dstBinding = 0;
        writeModelInfo.descriptorCount = 1;
        writeModelInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeModelInfo.pBufferInfo = &modelBufferInfo;

        VkWriteDescriptorSet writeDescriptorSets[] = { writeComputeInfo, writeCameraInfo, writeModelInfo };

        vkUpdateDescriptorSets(device->GetVulkanDevice(), 3, writeDescriptorSets, 0, nullptr);
    }

    VkRenderPass renderPass = CreateRenderPass();
    VkPipelineLayout computePipelineLayout = CreatePipelineLayout({ computeSetLayout });
    VkPipelineLayout graphicsPipelineLayout = CreatePipelineLayout({ cameraSetLayout, modelSetLayout });
    VkPipeline computePipeline = CreateComputePipeline(computePipelineLayout);
    VkPipeline graphicsPipeline = CreateGraphicsPipeline(graphicsPipelineLayout, renderPass, 0);

    // Create one framebuffer for each frame of the swap chain
    std::vector<VkFramebuffer> frameBuffers = CreateFrameBuffers(renderPass);

    // Create one command buffer for each frame of the swap chain
    std::vector<VkCommandBuffer> commandBuffers(swapchain->GetCount());

    // Specify the command pool and number of buffers to allocate
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(device->GetVulkanDevice(), &commandBufferAllocateInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers");
    }

    // Record command buffers, one for each frame of the swapchain
    for (unsigned int i = 0; i < commandBuffers.size(); ++i) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(commandBuffers[i], &beginInfo);

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = frameBuffers[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapchain->GetExtent();

        VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        // Bind the compute pipeline
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);

        // Bind descriptor sets for compute
        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeSet, 0, nullptr);

        // Dispatch the compute kernel, with one thread for each vertex
        vkCmdDispatch(commandBuffers[i], vertices.size(), 1, 1);

        // Define a memory barrier to transition the vertex buffer from a compute storage object to a vertex input
        VkBufferMemoryBarrier computeToVertexBarrier = {};
        computeToVertexBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        computeToVertexBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        computeToVertexBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        computeToVertexBarrier.srcQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Compute);
        computeToVertexBarrier.dstQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Graphics);
        computeToVertexBarrier.buffer = vertexBuffer;
        computeToVertexBarrier.offset = 0;
        computeToVertexBarrier.size = vertexBufferSize;

        vkCmdPipelineBarrier(commandBuffers[i],
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            0,
            0, nullptr,
            1, &computeToVertexBarrier,
            0, nullptr);

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Bind camera descriptor set
        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, 1, &cameraSet, 0, nullptr);

        // Bind the graphics pipeline
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        // Bind model descriptor set
        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 1, 1, &modelSet, 0, nullptr);

        VkDeviceSize offsets[1] = { 0 };
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &vertexBuffer, offsets);

        // Draw single point which will tessellate to a triangle
		vkCmdDraw(commandBuffers[i], 1, 1, 0, 1);

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer");
        }
    }

    glfwSetMouseButtonCallback(GetGLFWWindow(), mouseButtonCallback);
    glfwSetCursorPosCallback(GetGLFWWindow(), cursorPosCallback);
    glfwSetScrollCallback(GetGLFWWindow(), scrollCallback);

    int frameNumber = 0;
    // Map the part of the buffer referring the camera view matrix so it can be updated when the camera moves
    vkMapMemory(device->GetVulkanDevice(), uniformBufferMemory, uniformBufferOffsets[0] + offsetof(CameraUBO, viewMatrix), sizeof(glm::mat4), 0, reinterpret_cast<void**>(&mappedCameraView));
    while (!ShouldQuit()) {
        swapchain->Acquire();

        // Submit the command buffer
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { swapchain->GetImageAvailableSemaphore() };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[swapchain->GetIndex()];

        VkSemaphore signalSemaphores[] = { swapchain->GetRenderFinishedSemaphore() };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(device->GetQueue(QueueFlags::Graphics), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit draw command buffer");
        }

        swapchain->Present();

        glfwPollEvents();
    }
    vkUnmapMemory(device->GetVulkanDevice(), uniformBufferMemory);

    // Wait for the device to finish executing before cleanup
    vkDeviceWaitIdle(device->GetVulkanDevice());

    delete camera;

    vkDestroyBuffer(device->GetVulkanDevice(), vertexBuffer, nullptr);
    vkFreeMemory(device->GetVulkanDevice(), vertexBufferMemory, nullptr);

    vkDestroyBuffer(device->GetVulkanDevice(), cameraBuffer, nullptr);
    vkDestroyBuffer(device->GetVulkanDevice(), modelBuffer, nullptr);
    vkFreeMemory(device->GetVulkanDevice(), uniformBufferMemory, nullptr);

    vkDestroyDescriptorSetLayout(device->GetVulkanDevice(), computeSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(device->GetVulkanDevice(), cameraSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(device->GetVulkanDevice(), modelSetLayout, nullptr);
    vkDestroyDescriptorPool(device->GetVulkanDevice(), descriptorPool, nullptr);

    vkDestroyPipeline(device->GetVulkanDevice(), computePipeline, nullptr);
    vkDestroyPipelineLayout(device->GetVulkanDevice(), computePipelineLayout, nullptr);

    vkDestroyPipeline(device->GetVulkanDevice(), graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device->GetVulkanDevice(), graphicsPipelineLayout, nullptr);
    vkDestroyRenderPass(device->GetVulkanDevice(), renderPass, nullptr);
    vkFreeCommandBuffers(device->GetVulkanDevice(), commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
    for (size_t i = 0; i < frameBuffers.size(); i++) {
        vkDestroyFramebuffer(device->GetVulkanDevice(), frameBuffers[i], nullptr);
    }
    delete swapchain;

    vkDestroyCommandPool(device->GetVulkanDevice(), commandPool, nullptr);
    vkDestroySurfaceKHR(instance->GetVkInstance(), surface, nullptr);
    delete device;
    delete instance;

    DestroyWindow();
}