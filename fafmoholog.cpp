#include <shlobj_core.h>

#define USE_KTHOOKWNDPROC // scfa doesnt call standart wndproc for mouse events, need install custom hook
#include "imgui_hook.hpp"

namespace Moho
{
    kthook::kthook_simple<LRESULT(__thiscall *)(void *, UINT, WPARAM, LPARAM)> scfa_wndproc_hook{0x96D110};
    ImGui::Hook::D3D9 d3d9hook{};
    class Log
    {
        ImGuiTextBuffer Buf;
        ImGuiTextFilter Filter;
        ImVector<int> LineOffsets;
        bool AutoScroll;
        bool Render;

    public:
        Log()
        {
            AutoScroll = true;
            Clear();
            ImGui::Hook::OnFrame += [this]()
            {
                if (Render)
                    Draw("LOG", &Render);
            };
            ImGui::Hook::OnWndProc += [this](auto, auto msg, auto wParam, auto)
            {
                if (msg == WM_KEYDOWN && wParam == VK_F9) {
                        Render = !Render;
                        return 0; //1 = block key
                }
                else if (msg == WM_KEYUP && wParam == VK_F9)
                    return 0; //1 = block key
                return 0;
            };
            ImGui::Hook::OnInitialize += [this]() {
                scfa_wndproc_hook.set_cb([](const decltype(scfa_wndproc_hook) &hook, auto this_, auto uMsg, auto wParam, auto lParam) -> LPARAM {
                    if(ImGui::Hook::Input::WndProc(ImGui::Hook::Input::Window, uMsg, wParam, lParam))
                        return 1; //block msg
                    return hook.get_trampoline()(this_, uMsg, wParam, lParam); 
                });
                scfa_wndproc_hook.install();

                ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

                // add custom font and cyrillic characters
                ImVector<ImWchar> ranges;
                ImFontGlyphRangesBuilder builder;
                auto &io = ImGui::GetIO();

                std::string fontPath(MAX_PATH, '\0');
                if (!SHGetSpecialFolderPath(NULL, &fontPath[0], CSIDL_FONTS, FALSE))
                    throw;
                fontPath.resize(std::strlen(fontPath.c_str()));
                fontPath += "\\trebucbd.ttf";

                builder.AddText("вЂљвЂћвЂ¦вЂ вЂЎв‚¬вЂ°вЂ№вЂ�вЂ™вЂњвЂќвЂўвЂ“вЂ”в„ўвЂєв„–");
                builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
                builder.BuildRanges(&ranges);
                io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 14.0, NULL, ranges.Data);
                io.Fonts->Build();
            };
            d3d9hook.Install(ImGui::Hook::D3D9::Method::EndScene);
        }

        void Clear()
        {
            Buf.clear();
            LineOffsets.clear();
            LineOffsets.push_back(0);
        }

        void Add(const char *fmt, ...) IM_FMTARGS(2)
        {
            int old_size = Buf.size();
            va_list args;
            va_start(args, fmt);
            Buf.appendfv(fmt, args);
            va_end(args);
            for (int new_size = Buf.size(); old_size < new_size; old_size++)
                if (Buf[old_size] == '\n')
                    LineOffsets.push_back(old_size + 1);
        }

        void Draw(const char *title, bool *p_open = NULL)
        {
            if (!ImGui::Begin(title, p_open))
            {
                ImGui::End();
                return;
            }

            // Options menu
            if (ImGui::BeginPopup("Options"))
            {
                ImGui::Checkbox("Auto-scroll", &AutoScroll);
                ImGui::EndPopup();
            }

            // Main window
            if (ImGui::Button("Options"))
                ImGui::OpenPopup("Options");
            ImGui::SameLine();
            bool clear = ImGui::Button("Clear");
            ImGui::SameLine();
            bool copy = ImGui::Button("Copy");
            ImGui::SameLine();
            Filter.Draw("Filter", -100.0f);

            ImGui::Separator();
            ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

            if (clear)
                Clear();
            if (copy)
                ImGui::LogToClipboard();

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            const char *buf = Buf.begin();
            const char *buf_end = Buf.end();
            if (Filter.IsActive())
            {
                // In this example we don't use the clipper when Filter is enabled.
                // This is because we don't have a random access on the result on our filter.
                // A real application processing logs with ten of thousands of entries may want to store the result of
                // search/filter.. especially if the filtering function is not trivial (e.g. reg-exp).
                for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
                {
                    const char *line_start = buf + LineOffsets[line_no];
                    const char *line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                    if (Filter.PassFilter(line_start, line_end))
                        ImGui::TextUnformatted(line_start, line_end);
                }
            }
            else
            {
                ImGuiListClipper clipper;
                clipper.Begin(LineOffsets.Size);
                while (clipper.Step())
                {
                    for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                    {
                        const char *line_start = buf + LineOffsets[line_no];
                        const char *line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                        ImGui::TextUnformatted(line_start, line_end);
                    }
                }
                clipper.End();
            }
            ImGui::PopStyleVar();

            if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);

            ImGui::EndChild();
            ImGui::End();
        }
    } log;

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
        int alloc;
    };
    
    #define VALIDATE_SIZE(struc, size) static_assert(sizeof(struc) == size, "Invalid structure size of " #struc)
    VALIDATE_SIZE(scfafstring, 0x28);

    kthook::kthook_simple<void(__stdcall *)(int, scfafstring *)> kek{0x4F5AE0, [](const auto &hook, auto a1, auto a2)
        {
            auto str = a2->size > 16 ? a2->pStr : a2->str;
            if(a2->type == 0)
            {
                log.Add("[DEBUG]: %s\n", str);
            }
            else if(a2->type == 1)
            {
                log.Add("[LOG]: %s\n", str);
            }
            else if(a2->type == 2)
            {
                log.Add("[WARN]: %s\n", str);
            }
            else
            {
                log.Add("[ERROR]: %s\n", str);
            }
            return hook.get_trampoline()(a1, a2);
        }};
}