#pragma once

#include "rad/VulkanEngine/Framework/VulkanWindow.h"
#include "rad/IO/JsonValue.h"

enum class EffectType
{
    Undefined,
    ShaderToy,
};

class Effect : public rad::RefCounted<Effect>
{
public:
    Effect(VulkanWindow* window, EffectType type);
    ~Effect();

    EffectType GetType() const { return m_type; }

    // Set the root directory that resource path based on.
    void SetBaseDirectory(const rad::FilePath& baseDir);

    virtual bool Init(VulkanDevice* device, uint32_t backBufferCount);
    virtual bool Load(const rad::JsonValueRef jConfig) = 0;
    virtual void Render() = 0;

    VulkanCommandBuffer* BeginRender();
    void EndRender();

    virtual void Resize(uint32_t width, uint32_t height) {}

    VulkanImage* GetColorBuffer() { return m_colorBuffer.get(); }

    virtual void OnMouseButtonDown(const SDL_MouseButtonEvent& mouseButton) {}
    virtual void OnMouseButtonUp(const SDL_MouseButtonEvent& mouseButton) {}

protected:
    VulkanWindow* m_window = nullptr;
    EffectType m_type = EffectType::Undefined;
    VulkanDevice* m_device = nullptr;

    rad::Ref<VulkanCommandPool> m_commandPool;
    std::vector<rad::Ref<VulkanCommandBuffer>> m_commandBuffers;
    uint32_t m_backBufferIndex = 0;

    rad::FilePath m_baseDir;
    rad::Ref<VulkanImage> m_colorBuffer;

}; // class Effect
