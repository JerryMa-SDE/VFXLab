#include "Effect.h"
#include "Shadertoy/Shadertoy.h"

Effect::Effect(VulkanWindow* window, EffectType type) :
    m_window(window),
    m_type(type)
{
}

Effect::~Effect()
{
}

void Effect::SetBaseDirectory(const rad::FilePath& baseDir)
{
    m_baseDir = baseDir;
}

bool Effect::Init(VulkanDevice* device, uint32_t backBufferCount)
{
    m_device = device;
    m_commandPool = m_device->CreateCommandPool(VulkanQueueFamilyUniversal,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    m_commandBuffers.resize(backBufferCount);
    for (uint32_t i = 0; i < backBufferCount; i++)
    {
        m_commandBuffers[i] = m_commandPool->Allocate();
    }
    return true;
}

VulkanCommandBuffer* Effect::BeginRender()
{
    return m_commandBuffers[m_backBufferIndex].get();
}

void Effect::EndRender()
{
    m_backBufferIndex++;
    m_backBufferIndex %= m_commandBuffers.size();
}
