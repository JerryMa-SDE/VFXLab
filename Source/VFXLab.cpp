#include "VFXLab.h"
#include "EffectFactory.h"
#include "Logging.h"

VFXLab::VFXLab()
{
}

VFXLab::~VFXLab()
{
}

bool VFXLab::Init()
{
    float windowScale = GetDisplayDPI(0) / 96.0f;
    SDL_WindowFlags windowFlags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    VulkanWindow::Init("VFXLab",
        SDL_WINDOWPOS_CENTERED_DISPLAY(0), SDL_WINDOWPOS_CENTERED_DISPLAY(0),
        int(800 * windowScale), int(600 * windowScale),
        windowFlags);

    int width = 0;
    int height = 0;
    GetDrawableSize(&width, &height);

    m_renderTarget = m_device->CreateImage2DRenderTarget(VK_FORMAT_R8G8B8A8_UNORM,
        width, height, VK_IMAGE_USAGE_SAMPLED_BIT);
    m_depthStencil = m_device->CreateImage2DDepthStencil(VK_FORMAT_D32_SFLOAT_S8_UINT, width, height);

    m_frame = RAD_NEW VulkanFrame(this);
    m_frame->Init(m_device.get());
    m_backBufferCount = m_frame->GetBackBufferCount();
    m_guiContext = RAD_NEW VulkanGuiContext(this);
    m_guiContext->Init(m_device.get(), m_backBufferCount);

    float fontSize = GetDisplayDPI() / 72.0f * 12.0f;
#if defined(_WIN32)
    auto fonts = m_guiContext->GetFonts();
    fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", fontSize);
    m_guiContext->UploadFonts();
#endif

    m_frame->SetColorBuffer(m_renderTarget);
    m_frame->SetOverlay(m_guiContext->GetRenderTarget());

    const auto& args = DirectMedia::Application::GetInstance()->GetCommandLineArgs();
    if (args.size() > 1)
    {
        const rad::FilePath filePath = (const char8_t*)args[1].c_str();
        LoadEffect(filePath);
    }

    return true;
}

bool VFXLab::OnEvent(const SDL_Event& event)
{
    m_guiContext->ProcessEvent(event);
    return Window::OnEvent(event);
}

void VFXLab::OnShown()
{
    LogGlobal(Info, "OnShown");
}

void VFXLab::OnHidden()
{
    LogGlobal(Info, "OnHidden");
}

void VFXLab::OnExposed()
{
    LogGlobal(Info, "OnExposed");
}

void VFXLab::OnMoved(int x, int y)
{
    LogGlobal(Info, "OnMoved: %4d, %4d", x, y);
}

void VFXLab::OnResized(int width, int height)
{
    LogGlobal(Info, "OnResized: %4d, %4d", width, height);

    m_device->WaitIdle();
    GetDrawableSize(&width, &height);
    for (const auto& effect : m_effects)
    {
        effect->Resize(width, height);
    }

    m_guiContext->Resize(width, height);
    m_frame->Resize(width, height);

    m_frame->SetColorBuffer(m_effects.back()->GetColorBuffer());
    m_frame->SetOverlay(m_guiContext->GetRenderTarget());
}

void VFXLab::OnMinimized()
{
    LogGlobal(Info, "OnMinimized");
}

void VFXLab::OnMaximized()
{
    LogGlobal(Info, "OnMaximized");
}

void VFXLab::OnRestored()
{
    LogGlobal(Info, "OnRestored");
}

void VFXLab::OnEnter()
{
    LogGlobal(Info, "OnEnter");
}

void VFXLab::OnLeave()
{
    LogGlobal(Info, "OnLeave");
}

void VFXLab::OnKeyDown(const SDL_KeyboardEvent& keyDown)
{
    LogGlobal(Info, "OnKeyDown: %s", SDL_GetKeyName(keyDown.keysym.sym));

    if (keyDown.keysym.mod & KMOD_CTRL)
    {
        if (keyDown.keysym.sym == SDLK_o)
        {
            LogGlobal(Info, "OnKeyDown: Ctrl+O");
        }
    }

    if (keyDown.keysym.sym == SDLK_ESCAPE)
    {
        Destroy();
    }
}

void VFXLab::OnKeyUp(const SDL_KeyboardEvent& keyUp)
{
    LogGlobal(Info, "OnKeyUp: %s", SDL_GetKeyName(keyUp.keysym.sym));
}

void VFXLab::OnMouseMove(const SDL_MouseMotionEvent& mouseMotion)
{
}

void VFXLab::OnMouseButtonDown(const SDL_MouseButtonEvent& mouseButton)
{
    LogGlobal(Info, "OnMouseButtonDown: %s (%dx%d)",
        GetMouseButtonName(mouseButton.button), mouseButton.x, mouseButton.y);
    for (auto& effect : m_effects)
    {
        effect->OnMouseButtonDown(mouseButton);
    }
}

void VFXLab::OnMouseButtonUp(const SDL_MouseButtonEvent& mouseButton)
{
    LogGlobal(Info, "OnMouseButtonUp: %s (%dx%d)",
        GetMouseButtonName(mouseButton.button), mouseButton.x, mouseButton.y);
    for (auto& effect : m_effects)
    {
        effect->OnMouseButtonUp(mouseButton);
    }
}

void VFXLab::OnMouseWheel(const SDL_MouseWheelEvent& mouseWheel)
{
    LogGlobal(Info, "OnMouseWheel: %+d", mouseWheel.y);
}

void VFXLab::OnRender()
{
    bool isMinimized = this->IsMinimized();
    if (!isMinimized)
    {
        m_frame->Begin();
        for (const auto& effect : m_effects)
        {
            if (effect)
            {
                effect->Render();
            }
        }
        m_guiContext->NewFrame();
        m_guiContext->Render();
        m_guiContext->Submit();
        m_frame->End();
    }
}

const char* VFXLab::GetMouseButtonName(Uint8 button)
{
    switch (button)
    {
    case SDL_BUTTON_LEFT: return "SDL_BUTTON_LEFT";
    case SDL_BUTTON_MIDDLE: return "SDL_BUTTON_MIDDLE";
    case SDL_BUTTON_RIGHT: return "SDL_BUTTON_RIGHT";
    case SDL_BUTTON_X1: return "SDL_BUTTON_X1";
    case SDL_BUTTON_X2: return "SDL_BUTTON_X2";
    }
    return "SDL_BUTTON_UNKNOWN";
}

bool VFXLab::LoadEffect(const rad::FilePath& filePath)
{
    m_doc = RAD_NEW rad::JsonDoc();
    if (rad::Exists(filePath) && m_doc->ParseFile(filePath))
    {
        rad::FilePath baseDir = rad::MakeAbsolute(filePath).remove_filename();
        rad::JsonValueRef jRoot = m_doc->GetRoot();
        rad::JsonValueRef jEffectNames = jRoot["Effects"];
        if (jEffectNames && jEffectNames.IsArray())
        {
            for (const auto& jEffectName : jEffectNames.GetArray())
            {
                rad::JsonValueRef jEffect = jRoot[jEffectName.GetString()];
                rad::Ref<Effect> effect = CreateEffect(this, jEffect["Type"].GetString());
                if (effect)
                {
                    effect->Init(m_device.get(), m_backBufferCount);
                    effect->SetBaseDirectory(baseDir);
                    effect->Load(jEffect);
                    m_effects.push_back(std::move(effect));
                }
            }
        }
    }

    if (!m_effects.empty())
    {
        m_frame->SetColorBuffer(m_effects.back()->GetColorBuffer());
    }

    return true;
}
