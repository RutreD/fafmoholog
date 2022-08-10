#include <shlobj_core.h>

#define MINI_CASE_SENSITIVE
#include "ini.h"

#define USE_KTHOOKWNDPROC // scfa doesn't call standard wndproc for mouse events, need install custom hook
#include "imgui_hook.hpp"

namespace Moho
{
    kthook::kthook_simple<LRESULT(__thiscall *)(void *, UINT, WPARAM, LPARAM)> scfa_wndproc_hook{0x96D110};
    ImGui::Hook::D3D9 d3d9hook{};

    class Log
    {
        mINI::INIFile file{"imgui.ini"};
        mINI::INIStructure ini;
        ImGuiTextBuffer Buf;
        ImGuiTextFilter Filter;
        ImVector<int> LineOffsets;
        bool AutoScroll;
        bool Render;
        bool SelectionMode;
        float lastScrollY;
        ImVector<char *> FontsArray;
        int FontIndex = 0;
        const char *FontName;
        float FontSize = 14.0f;
        bool FontChanged;

    public:
        Log();
        ~Log();

        void ApplyFont(const char *fontName, float size = 14.0f);
        void FindFonts();
        void LoadSettings();
        void SaveSettings();
        void Draw(const char *title, bool *p_open = NULL);
        void Add(const char *fmt, ...);
        void Clear();
    };

    struct scfafstring
    {
        void *dword0;
        void *dword4;
        int type;
        void *dwordC;
        union
        {
            char *pStr;
            char str[16];
        };
        int size;
        int allocated;
    };

    static_assert(sizeof(scfafstring) == 0x28, "Invalid structure size of scfafstring");
}
