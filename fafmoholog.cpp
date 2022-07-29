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
        bool SelectMode;
        float lastScrollY;

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
                if (msg == WM_KEYDOWN && wParam == VK_F9)
                {
                    Render = !Render;
                    return 0; // 1 = block key
                }
                else if (msg == WM_KEYUP && wParam == VK_F9)
                    return 0; // 1 = block key
                return 0;
            };
            ImGui::Hook::OnInitialize += [this]()
            {
                scfa_wndproc_hook.set_cb([](const decltype(scfa_wndproc_hook) &hook, auto this_, auto uMsg, auto wParam, auto lParam) -> LPARAM {
                    if(ImGui::Hook::Input::WndProc(ImGui::Hook::Input::Window, uMsg, wParam, lParam))
                        return 1; //block msg
                    return hook.get_trampoline()(this_, uMsg, wParam, lParam); });
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
            Buf.append("");
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
            if (!ImGui::Begin(title, p_open, ImGuiWindowFlags_NoScrollbar))
            {
                ImGui::End();
                return;
            }

            ImGui::Checkbox("Auto-scroll", &AutoScroll);
            ImGui::SameLine();
            if (ImGui::Button("Clear log"))
                Clear();
            ImGui::SameLine();
            bool copy = ImGui::Button("Copy log");

            ImGui::SameLine(ImGui::GetWindowWidth() - 20);
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted("Guide");
                if (ImGui::BeginTable("Guide", 2))
                {
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Click on a line");
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Copy the line");
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Right click or double click");
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Selection mode");
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Escape or right click");
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Cancel selection mode");
                    ImGui::EndTable();
                }
                ImGui::TextUnformatted("Filter usage");
                if (ImGui::BeginTable("Filter usage", 2))
                {
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("\"\"");
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Display all lines");
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("\"xxx\"");
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Display lines containing \"xxx\"");
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("\"xxx,yyy\"");
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Display lines containing \"xxx\" or \"yyy\"");
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("\"-xxx\"");
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Hide lines containing \"xxx\"");
                    ImGui::EndTable();
                }
                ImGui::EndTooltip();
            }

            Filter.Draw("Filter", -280);

            ImGui::SameLine();
            if (ImGui::Button("Log"))
            {
                if (Filter.InputBuf[0] != '\0')
                    strcpy_s(Filter.InputBuf + strlen(Filter.InputBuf), 256 - strlen(Filter.InputBuf), ",");
                strcpy_s(Filter.InputBuf + strlen(Filter.InputBuf), 256 - strlen(Filter.InputBuf), "[LOG]");
                Filter.Build();
            }
            ImGui::SameLine();
            if (ImGui::Button("Warn"))
            {
                if (Filter.InputBuf[0] != '\0')
                    strcpy_s(Filter.InputBuf + strlen(Filter.InputBuf), 256 - strlen(Filter.InputBuf), ",");
                strcpy_s(Filter.InputBuf + strlen(Filter.InputBuf), 256 - strlen(Filter.InputBuf), "[WARN]");
                Filter.Build();
            }
            ImGui::SameLine();
            if (ImGui::Button("Debug"))
            {
                if (Filter.InputBuf[0] != '\0')
                    strcpy_s(Filter.InputBuf + strlen(Filter.InputBuf), 256 - strlen(Filter.InputBuf), ",");
                strcpy_s(Filter.InputBuf + strlen(Filter.InputBuf), 256 - strlen(Filter.InputBuf), "[DEBUG]");
                Filter.Build();
            }
            ImGui::SameLine();
            if (ImGui::Button("Error"))
            {
                if (Filter.InputBuf[0] != '\0')
                    strcpy_s(Filter.InputBuf + strlen(Filter.InputBuf), 256 - strlen(Filter.InputBuf), ",");
                strcpy_s(Filter.InputBuf + strlen(Filter.InputBuf), 256 - strlen(Filter.InputBuf), "[ERROR]");
                Filter.Build();
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear"))
                Filter.Clear();
            ImGui::Separator();

            if (copy)
                ImGui::LogToClipboard();

            if (!SelectMode)
            {
                ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                const char *buf = Buf.begin();
                const char *buf_end = Buf.end();
                if (Filter.IsActive())
                {
                    for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
                    {
                        const char *line_start = buf + LineOffsets[line_no];
                        const char *line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                        if (Filter.PassFilter(line_start, line_end))
                        {
                            if (line_start[1] == 'D')
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                            else if (line_start[1] == 'E')
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.0f, 0.0f, 1.0f));
                            else if (line_start[1] == 'W')
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.0f, 1.0f));
                            ImGui::TextUnformatted(line_start, line_end);
                            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                            {
                                int len = static_cast<int>(line_end - line_start);
                                char *buffer = new char[len + 1];
                                memcpy(buffer, line_start, len);
                                buffer[len] = '\0';
                                ImGui::SetClipboardText(buffer);
                                delete[] buffer;
                            }
                            if (line_start[1] == 'D' || line_start[1] == 'E' || line_start[1] == 'W')
                                ImGui::PopStyleColor(1);
                        }
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
                            if (line_start[1] == 'D')
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                            else if (line_start[1] == 'E')
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.0f, 0.0f, 1.0f));
                            else if (line_start[1] == 'W')
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.0f, 1.0f));
                            ImGui::TextUnformatted(line_start, line_end);
                            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                            {
                                int len = static_cast<int>(line_end - line_start);
                                char *buffer = new char[len + 1];
                                memcpy(buffer, line_start, len);
                                buffer[len] = '\0';
                                ImGui::SetClipboardText(buffer);
                                delete[] buffer;
                            }
                            if (line_start[1] == 'D' || line_start[1] == 'E' || line_start[1] == 'W')
                                ImGui::PopStyleColor(1);
                        }
                    }
                    clipper.End();
                }
                ImGui::PopStyleVar();
                if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                    ImGui::SetScrollHereY(1.0f);
                if (lastScrollY != 0)
                {
                    ImGui::SetScrollY(lastScrollY);
                    lastScrollY = 0;
                }
                if (!SelectMode && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)))
                {
                    SelectMode = true;
                    lastScrollY = ImGui::GetScrollY() + 20;
                }
                ImGui::EndChild();
            }
            else
            {
                ImGui::InputTextMultiline("##log", Buf.Buf.begin(), Buf.size(), ImVec2(-1, -15), ImGuiInputTextFlags_ReadOnly);
                ImGui::BeginChild("##log", ImVec2(1, 1), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
                if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                    ImGui::SetScrollHereY(1.0f);
                if (lastScrollY != 0)
                {
                    ImGui::SetScrollY(lastScrollY);
                    lastScrollY = 0;
                }
                if (SelectMode && (ImGui::IsKeyDown(ImGuiKey_Escape) || ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)))
                {
                    SelectMode = false;
                    lastScrollY = ImGui::GetScrollY();
                }
                ImGui::EndChild();
            }
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
        if (a2->type == 0)
        {
            log.Add("[DEBUG]: %s\n", str);
        }
        else if (a2->type == 1)
        {
            log.Add("[LOG]: %s\n", str);
        }
        else if (a2->type == 2)
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