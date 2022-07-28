#if !defined(IMGUI_HOOK_HPP)
#define IMGUI_HOOK_HPP

#include <d3d9.h>
#include "kthook/kthook.hpp"
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace ImGui::Hook
{
    /// Call after ImGui inited
    ktsignal::ktsignal<void()> OnInitialize{};
    /// Call after ImGui::NewFrame();
    ktsignal::ktsignal<void()> OnFrame{};
    /// Call before Reset()
    ktsignal::ktsignal<void()> OnReset{};
    /// Call after Reset()
    ktsignal::ktsignal<void()> OnAfterReset{};
    /// Call before Present()
    ktsignal::ktsignal<void()> OnPresent{};
    /// Call before EndScene()
    ktsignal::ktsignal<void()> OnEndScene{};
    /// Call before WndProc
    ktsignal::ktsignal<LRESULT(HWND, UINT, WPARAM, LPARAM)> OnWndProc;

    class Input
    {
#if !defined(USE_KTHOOKWNDPROC)
    private:
        static inline WNDPROC OrigWndProc;
#else
    public:
#endif
        static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    public:
        static inline HWND Window;
#if !defined(USE_KTHOOKWNDPROC)
        void Install(HWND hWnd = Window);
        Input();
        ~Input();
#endif
    };
    LRESULT CALLBACK Input::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
            return 1;
        if ((uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) || (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST))
        {
            auto &io = ImGui::GetIO();
            if (io.WantCaptureMouse || io.WantCaptureKeyboard)
                return 1;
        }
        for (auto res : OnWndProc.emit_iterate(hWnd, uMsg, wParam, lParam))
        {
            if (res)
                return 1;
        }
#if !defined(USE_KTHOOKWNDPROC)
        return CallWindowProc(OrigWndProc, hWnd, uMsg, wParam, lParam);
#else
        return 0;
#endif
    }
#if !defined(USE_KTHOOKWNDPROC)
    void Input::Install(HWND hWnd)
    {
        if (OrigWndProc)
            return;
        OrigWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG>(&WndProc)));
    }
    Input::Input() {}
    Input::~Input()
    {
        if (GetWindowLongPtr(Window, GWLP_WNDPROC) != reinterpret_cast<LONG>(&WndProc))
            return;
        SetWindowLongPtr(Window, GWLP_WNDPROC, reinterpret_cast<LONG>(OrigWndProc));
    }
#endif

    class D3D9
    {
    private:
        static inline D3D9 *instance;
        static inline bool init{false};
        kthook::kthook_simple<HRESULT(__stdcall *)(IDirect3DDevice9 *, D3DPRESENT_PARAMETERS *)> reset_hook;
        kthook::kthook_simple<HRESULT(__stdcall *)(IDirect3DDevice9 *, const RECT *, const RECT *, HWND, const RGNDATA *)> present_hook;
        kthook::kthook_simple<HRESULT(__stdcall *)(IDirect3DDevice9 *)> endscene_hook;

        static HRESULT present_hooked(const decltype(present_hook) &hook, IDirect3DDevice9 *pDevice, const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion);
        static HRESULT endscene_hooked(const decltype(endscene_hook) &hook, IDirect3DDevice9 *pDevice);

        static HRESULT reset_hooked(const decltype(reset_hook) &hook, IDirect3DDevice9 *pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters);

    protected:
        virtual inline void Init(IDirect3DDevice9 *pDevice);
        virtual inline void RenderFrame();

    public:
        static inline LPDIRECT3DDEVICE9 Device;
        static std::uintptr_t *GetVirtualTable(std::uintptr_t *default_vtable = 0);

        enum class Method
        {
            Present,
            EndScene
        };
        bool Install(Method method);

        D3D9(std::uintptr_t *vtable = GetVirtualTable());
        virtual ~D3D9();
    };

    HRESULT D3D9::present_hooked(const decltype(present_hook) &hook, IDirect3DDevice9 *pDevice, const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion)
    {
        OnPresent.emit();
        if (!init)
            instance->Init(pDevice);
        instance->RenderFrame();
        return hook.get_trampoline()(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
    }

    HRESULT D3D9::endscene_hooked(const decltype(endscene_hook) &hook, IDirect3DDevice9 *pDevice)
    {
        OnEndScene.emit();
        if (!init)
            instance->Init(pDevice);
        instance->RenderFrame();
        return hook.get_trampoline()(pDevice);
    }

    HRESULT D3D9::reset_hooked(const decltype(reset_hook) &hook, IDirect3DDevice9 *pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters)
    {
        OnReset.emit();
        ImGui_ImplDX9_InvalidateDeviceObjects();
        auto res = hook.get_trampoline()(pDevice, pPresentationParameters);
        OnAfterReset.emit();
        return res;
    }

    void D3D9::Init(IDirect3DDevice9 *pDevice)
    {
        if (!init)
        {
            Device = pDevice;
            D3DDEVICE_CREATION_PARAMETERS params;
            pDevice->GetCreationParameters(&params);
            ImGui::CreateContext();
            ImGui_ImplWin32_Init(params.hFocusWindow);
            ImGui_ImplDX9_Init(pDevice);
            ImGui::Hook::Input::Window = params.hFocusWindow;
            OnInitialize.emit();
            init = true;
        }
    }
    void D3D9::RenderFrame()
    {
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        OnFrame.emit();
        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    }
    std::uintptr_t *D3D9::GetVirtualTable(std::uintptr_t *default_vtable)
    {
        static std::uintptr_t *vt = [&]()
        {
            std::string path_to(MAX_PATH, '\0');
            if (auto size = GetSystemDirectoryA(path_to.data(), MAX_PATH))
            {
                path_to.resize(size);
                path_to += "\\d3d9.dll";
                std::uintptr_t dwObjBase = reinterpret_cast<std::uintptr_t>(LoadLibraryA(path_to.c_str()));
                while (dwObjBase++ < dwObjBase + 0x128000)
                {
                    if (*reinterpret_cast<std::uint16_t *>(dwObjBase + 0x00) == 0x06C7 &&
                        *reinterpret_cast<std::uint16_t *>(dwObjBase + 0x06) == 0x8689 &&
                        *reinterpret_cast<std::uint16_t *>(dwObjBase + 0x0C) == 0x8689)
                    {
                        dwObjBase += 2;
                        return *reinterpret_cast<std::uintptr_t **>(dwObjBase);
                    }
                }
            }
            if (default_vtable == 0)
                throw "No vtbl";
            return default_vtable;
        }();
        return vt;
    }
    bool D3D9::Install(Method method)
    {
        if (method == Method::Present && !present_hook.install())
            return false;
        else if (method == Method::EndScene && !endscene_hook.install())
            return false;
        if (!reset_hook.install())
            return false;
        return true;
    }
    D3D9::D3D9(std::uintptr_t *vtable)
    {
        if (instance)
            throw;
        present_hook.set_dest(vtable[17]);
        present_hook.set_cb(&present_hooked);
        endscene_hook.set_dest(vtable[42]);
        endscene_hook.set_cb(&endscene_hooked);
        reset_hook.set_dest(vtable[16]);
        reset_hook.set_cb(&reset_hooked);
        instance = this;
    }
    D3D9::~D3D9()
    {
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
}
#endif