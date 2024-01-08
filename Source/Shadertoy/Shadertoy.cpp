#include "Shadertoy.h"
#include "Logging.h"
#include "rad/VulkanEngine/Rendering/VulkanTexture.h"

Shadertoy::Shadertoy(VulkanWindow* window) :
    Effect(window, EffectType::ShaderToy)
{
}

Shadertoy::~Shadertoy()
{
}

void Shadertoy::Render()
{
    if (m_resetTime)
    {
        m_startTime = Clock::now();
        m_lastTime = Clock::now();
        m_frameIndex = 0;
        m_resetTime = false;
    }
    Clock::time_point currTime = Clock::now();
    std::time_t t = std::time(nullptr);
    std::tm date = {};
    localtime_s(&date, &t);
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currTime - m_startTime).count();
    auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(currTime - m_lastTime).count();
    float time = float(duration) / 1000.0f;
    float deltaTime = float(frameDuration) / 1000.0f;

    VulkanCommandBuffer* cmdBuffer = BeginRender();

    cmdBuffer->Begin();

    for (const std::string& passName : m_passNames)
    {
        ShaderPass& pass = m_passes[passName];
        int32_t bufferIndex = m_frameIndex % 2;

        pass.uniforms.iResolution =
        {
            float(m_renderWidth),
            float(m_renderHeight),
            0.0f,
            0.0f,
        };
        pass.uniforms.iTime = time;
        pass.uniforms.iTimeDelta = deltaTime;
        pass.uniforms.iFrameRate = 1.0f / deltaTime;
        pass.uniforms.iFrame = m_frameIndex;
        pass.uniforms.iChannelTime = glm::vec4(time);
        for (uint32_t channelIndex = 0; channelIndex < pass.channels.size(); ++channelIndex)
        {
            VulkanImage* image = pass.channels[channelIndex].images[bufferIndex];
            if (image)
            {
                pass.uniforms.iChannelResolution[channelIndex] = glm::vec4(
                    float(image->GetWidth()),
                    float(image->GetHeight()),
                    0.0f,
                    0.0f
                );
            }
        }
        int mouseX = 0;
        int mouseY = 0;
        Uint32 mouseButton = SDL_GetMouseState(&mouseX, &mouseY);
        if (m_renderRatioX != 1.0f)
        {
            mouseX = (int)std::round(float(mouseX) * m_renderRatioX);
        }
        if (m_renderRatioY != 1.0f)
        {
            mouseY = (int)std::round(float(mouseY) * m_renderRatioY);
        }
        mouseY = int(m_renderHeight) - mouseY;
        if (m_mouseButtonDown)
        {
            pass.uniforms.iMouse.x = float(mouseX);
            pass.uniforms.iMouse.y = float(mouseY);
            pass.uniforms.iMouse.z = float(mouseX);
            if (m_mouseClick)
            {
                pass.uniforms.iMouse.w = float(mouseY);
                m_mouseClick = false;
            }
            else
            {
                pass.uniforms.iMouse.w = 0.0f;
            }
        }
        else
        {
            pass.uniforms.iMouse.z = 0.0f;
            pass.uniforms.iMouse.w = 0.0f;
        }
        pass.uniforms.iDate = glm::vec4(date.tm_year, date.tm_mon, date.tm_mday,
            date.tm_hour * 60 * 60 + date.tm_min * 60 + date.tm_sec);
        pass.uniformBuffer->Write(&pass.uniforms);

        for (uint32_t channelIndex = 0; channelIndex < pass.channels.size(); ++channelIndex)
        {
            VulkanImage* image = pass.channels[channelIndex].images[bufferIndex];
            if (image)
            {
                if (pass.channels[channelIndex].type == ChannelType::RenderTarget &&
                    image->GetCurrentLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                {
                    cmdBuffer->TransitLayoutFromCurrent(image,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        VK_ACCESS_SHADER_READ_BIT,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                }
            }
        }

        VulkanImage* colorImage = pass.colorBuffers[bufferIndex].get();
        if (colorImage->GetCurrentLayout() != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            cmdBuffer->TransitLayoutFromCurrent(colorImage,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }

        VkClearValue clearValues[1] = {};
        clearValues[0].color = {};
        if (m_useDynamicRendering)
        {
            cmdBuffer->BeginRendering(pass.colorBuffers[bufferIndex]->GetDefaultView(), nullptr);
        }
        else
        {
            cmdBuffer->BeginRenderPass(m_renderPass.get(), pass.framebuffers[bufferIndex].get(),
                clearValues);
        }
        cmdBuffer->BindPipeline(pass.pipeline.get());
        cmdBuffer->BindDescriptorSets(pass.pipeline.get(), m_pipelineLayout.get(),
            0, pass.descriptorSets[bufferIndex].get());

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = float(pass.colorBuffers[bufferIndex]->GetWidth());
        viewport.height = float(pass.colorBuffers[bufferIndex]->GetHeight());
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        cmdBuffer->SetViewports(viewport);

        VkRect2D scissor = {};
        scissor.extent.width = pass.colorBuffers[bufferIndex]->GetWidth();
        scissor.extent.height = pass.colorBuffers[bufferIndex]->GetHeight();
        cmdBuffer->SetScissors(scissor);
        cmdBuffer->Draw(3, 1, 0, 0);

        if (m_useDynamicRendering)
        {
            cmdBuffer->EndRendering();
        }
        else
        {
            cmdBuffer->EndRenderPass();
        }

        pass.colorBuffers[bufferIndex]->SetCurrentPipelineStage(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        pass.colorBuffers[bufferIndex]->SetCurrentAccessFlags(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
        pass.colorBuffers[bufferIndex]->SetCurrentLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    cmdBuffer->End();

    m_device->GetQueue()->Submit(cmdBuffer);
    EndRender();

    m_frameIndex++;
    m_lastTime = currTime;
}

void Shadertoy::Resize(uint32_t width, uint32_t height)
{
    CreateColorBuffers(width, height);
    UpdateResourceBindings();
    m_resetTime = true;
}

void Shadertoy::OnMouseButtonDown(const SDL_MouseButtonEvent& mouseButton)
{
    m_mouseButtonDown = true;
    m_mouseClick = true;
}

void Shadertoy::OnMouseButtonUp(const SDL_MouseButtonEvent& mouseButton)
{
    m_mouseButtonDown = false;
}

void Shadertoy::CreateResources()
{
    // create textures.
    for (const std::string& passName : m_passNames)
    {
        ShaderPass& pass = m_passes[passName];
        pass.uniformBuffer = m_device->CreateUniformBuffer(sizeof(UniformData), true);
        for (size_t i = 0; i < pass.channels.size(); ++i)
        {
            const ChannelInfo& channelInfo = pass.channels[i];
            if (channelInfo.type == ChannelType::Texture2D)
            {
                if (!m_textures.contains(channelInfo.name))
                {
                    rad::FilePath imagePath = m_baseDir /
                        (const char8_t*)channelInfo.name.c_str();
                    if (rad::Exists(imagePath))
                    {
                        m_textures[channelInfo.name] = CreateVulkanImage2DFromFile(m_device,
                            (const char*)imagePath.u8string().c_str(), true);
                    }
                    else
                    {
                        LogVFX(Error, "Cannot find image: %s",
                            (const char*)imagePath.u8string().c_str());
                    }
                }
            }
        }
    }

    // create samplers.
    m_samplerNearestClamp = m_device->CreatSamplerNearest(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    m_samplerNearestRepeat = m_device->CreatSamplerNearest(VK_SAMPLER_ADDRESS_MODE_REPEAT);
    m_samplerLinearClamp = m_device->CreatSamplerLinear(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    m_samplerLinearRepeat = m_device->CreatSamplerLinear(VK_SAMPLER_ADDRESS_MODE_REPEAT);
}

void Shadertoy::CreateRenderPasses()
{
    VkAttachmentDescription colorAttach = {};
    colorAttach.flags = 0;
    colorAttach.format = m_colorFormat;
    colorAttach.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttach.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttach.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttach.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef = {};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDesc = {};
    subpassDesc.flags = 0;
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &colorRef;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pNext = nullptr;
    renderPassInfo.flags = 0;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttach;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDesc;
    renderPassInfo.dependencyCount = 0;
    renderPassInfo.pDependencies = nullptr;

    m_renderPass = m_device->CreateRenderPass(renderPassInfo);
}

void Shadertoy::CreateColorBuffers(uint32_t width, uint32_t height)
{
    m_renderWidth = uint32_t(width * m_renderRatioX);
    m_renderHeight = uint32_t(height * m_renderRatioY);
    m_renderWidth = rad::Pow2AlignUp(m_renderWidth, 2);
    m_renderHeight = rad::Pow2AlignUp(m_renderHeight, 2);
    LogVFX(Info, "Shadertoy: render size %ux%u (%.2fx%.2f)",
        m_renderWidth, m_renderHeight, m_renderRatioX, m_renderRatioY);

    if (m_passNames.empty() || m_passes.empty())
    {
        return;
    }

    for (size_t i = 0; i < m_passNames.size() - 1; ++i)
    {
        const std::string& passName = m_passNames[i];
        ShaderPass& pass = m_passes[passName];

        for (uint32_t bufferIndex = 0; bufferIndex < 2; ++bufferIndex)
        {
            pass.colorBuffers[bufferIndex] = m_device->CreateImage2DRenderTarget(
                VK_FORMAT_R8G8B8A8_UNORM, m_renderWidth, m_renderHeight,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT // color buffer can be sampled as texture
            );
            if (!m_useDynamicRendering)
            {
                pass.framebuffers[bufferIndex] = m_device->CreateFramebuffer(m_renderPass.get(),
                    pass.colorBuffers[bufferIndex]->GetDefaultView(),
                    m_renderWidth,
                    m_renderHeight
                );
            }
        }
    }

    // The last pass only has one color target (no ping-pong buffering, cannot be sampled by any shader pass).
    ShaderPass& lastPass = m_passes[m_passNames.back()];
    m_colorBuffer = m_device->CreateImage2DRenderTarget(
        VK_FORMAT_R8G8B8A8_UNORM, m_renderWidth, m_renderHeight,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT // can be sampled by other effects or as color buffer of the frame.
    );
    lastPass.colorBuffers[0] = m_colorBuffer;
    lastPass.colorBuffers[1] = m_colorBuffer;
    if (!m_useDynamicRendering)
    {
        lastPass.framebuffers[0] = m_device->CreateFramebuffer(m_renderPass.get(),
            lastPass.colorBuffers[0]->GetDefaultView(),
            m_renderWidth,
            m_renderHeight
        );
        lastPass.framebuffers[1] = lastPass.framebuffers[0];
    }
}

void Shadertoy::CreatePipelines()
{
    uint32_t maxSets = MaxShaderPassCount * 2;
    m_descriptorPool = m_device->CreateDescriptorPool(maxSets,
        {
            VkDescriptorPoolSize
            {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = MaxShaderPassCount * 2,
            },
            VkDescriptorPoolSize
            {
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = MaxShaderPassCount * 4 * 2,
            },
        }
    );

    m_descriptorSetLayout = m_device->CreateDescriptorSetLayout(
        {
            VkDescriptorSetLayoutBinding
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_ALL,
                .pImmutableSamplers = nullptr,
            },
            VkDescriptorSetLayoutBinding
            {
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr,
            },
            VkDescriptorSetLayoutBinding
            {
                .binding = 2,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr,
            },
            VkDescriptorSetLayoutBinding
            {
                .binding = 3,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr,
            },
            VkDescriptorSetLayoutBinding
            {
                .binding = 4,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr,
            },
        }
    );

    m_pipelineLayout = m_device->CreatePipelineLayout(m_descriptorSetLayout.get());

    for (const std::string& passName : m_passNames)
    {
        ShaderPass& pass = m_passes[passName];

        rad::Ref<VulkanGraphicsPipelineCreateInfo> createInfo = RAD_NEW VulkanGraphicsPipelineCreateInfo();
        rad::Ref<VulkanShader> vert = VulkanShader::CreateFromFile(VK_SHADER_STAGE_VERTEX_BIT,
            "Shaders/Shadertoy.vert", "main");
        std::string fragSource = rad::File::ReadAll("Shaders/Shadertoy.frag") +
            rad::File::ReadAll(m_baseDir / (const char8_t*)pass.shaderName.c_str());
        rad::Ref<VulkanShader> frag = RAD_NEW VulkanShader();
        frag->Compile(VK_SHADER_STAGE_FRAGMENT_BIT,
            pass.shaderName, fragSource, "main", pass.shaderMacros);
        createInfo->m_shaders.push_back(vert);
        createInfo->m_shaders.push_back(frag);
        createInfo->m_inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        createInfo->SetColorBlendDisabled(0);
        createInfo->m_layout = m_pipelineLayout;
        if (m_useDynamicRendering)
        {
            createInfo->SetRenderingInfo(m_colorFormat);
        }
        else
        {
            createInfo->m_renderPass = m_renderPass;
        }
        pass.pipeline = m_device->CreateGraphicsPipeline(createInfo);

        for (size_t bufferIndex = 0; bufferIndex < 2; ++bufferIndex)
        {
            pass.descriptorSets[bufferIndex] = m_descriptorPool->Allocate(m_descriptorSetLayout.get());
        }
    }
}

void Shadertoy::UpdateResourceBindings()
{
    for (const std::string& passName : m_passNames)
    {
        ShaderPass& pass = m_passes[passName];
        for (size_t bufferIndex = 0; bufferIndex < 2; ++bufferIndex)
        {
            pass.descriptorSets[bufferIndex]->
                UpdateBuffers(0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, pass.uniformBuffer.get());
            for (uint32_t channelIndex = 0; channelIndex < pass.channels.size(); ++channelIndex)
            {
                ChannelInfo& channelInfo = pass.channels[channelIndex];
                VulkanImage* image = nullptr;
                if (channelInfo.type == ChannelType::Texture2D)
                {
                    auto iter = m_textures.find(channelInfo.name);
                    if (iter != m_textures.end())
                    {
                        image = iter->second.get();
                    }
                }
                else if (channelInfo.type == ChannelType::RenderTarget)
                {
                    auto iter = m_passes.find(channelInfo.name);
                    if (iter != m_passes.end())
                    {
                        // ping-pong buffering, sample the other one.
                        image = iter->second.colorBuffers[(bufferIndex + 1) % 2].get();
                    }
                }
                channelInfo.images[bufferIndex] = image;

                VulkanSampler* sampler = m_samplerLinearRepeat.get();
                if (channelInfo.filter == ChannelFilter::Nearest)
                {
                    if (channelInfo.wrap == ChannelWrap::Clamp)
                    {
                        sampler = m_samplerNearestClamp.get();
                    }
                    else if (channelInfo.wrap == ChannelWrap::Repeat)
                    {
                        sampler = m_samplerNearestRepeat.get();
                    }
                }
                else if (channelInfo.filter == ChannelFilter::Linear)
                {
                    if (channelInfo.wrap == ChannelWrap::Clamp)
                    {
                        sampler = m_samplerLinearClamp.get();
                    }
                    else if (channelInfo.wrap == ChannelWrap::Repeat)
                    {
                        sampler = m_samplerLinearRepeat.get();
                    }
                }

                if (image && sampler)
                {
                    pass.descriptorSets[bufferIndex]->UpdateCombinedImageSamplers(channelIndex + 1, 0,
                        image->GetDefaultView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler);
                }
            }
        }
    }
}
