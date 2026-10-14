#include "ui/launcher_theme.hpp"

namespace pit {
namespace ui {

Palette g_pal = {
    /* bg_top     */ ImVec4(0.055f, 0.075f, 0.188f, 1.0f),
    /* bg_bottom  */ ImVec4(0.027f, 0.035f, 0.071f, 1.0f),
    /* sidebar    */ ImVec4(0.071f, 0.102f, 0.219f, 1.0f),
    /* sidebar_hi */ ImVec4(0.125f, 0.180f, 0.360f, 1.0f),
    /* card       */ ImVec4(0.078f, 0.110f, 0.219f, 1.0f),
    /* card_header*/ ImVec4(0.118f, 0.165f, 0.314f, 1.0f),
    /* well       */ ImVec4(0.039f, 0.059f, 0.133f, 1.0f),
    /* edge       */ ImVec4(0.173f, 0.231f, 0.400f, 1.0f),
    /* gold       */ ImVec4(0.949f, 0.698f, 0.200f, 1.0f),
    /* gold_dim   */ ImVec4(0.604f, 0.420f, 0.078f, 1.0f),
    /* gold_bright*/ ImVec4(1.000f, 0.843f, 0.400f, 1.0f),
    /* text       */ ImVec4(0.957f, 0.937f, 0.878f, 1.0f),
    /* text_dim   */ ImVec4(0.576f, 0.639f, 0.776f, 1.0f),
    /* red        */ ImVec4(0.910f, 0.282f, 0.247f, 1.0f),
    /* green      */ ImVec4(0.310f, 0.761f, 0.294f, 1.0f),
    /* amber      */ ImVec4(0.906f, 0.639f, 0.235f, 1.0f),
};

ImU32 U32(const ImVec4 &c)
{
    return ImGui::ColorConvertFloat4ToU32(c);
}

ImU32 U32a(const ImVec4 &c, float alpha)
{
    ImVec4 t = c;
    t.w = alpha;
    return ImGui::ColorConvertFloat4ToU32(t);
}

void ApplyStyle()
{
    ImGuiStyle &s = ImGui::GetStyle();

    s.WindowRounding    = 0.0f;
    s.ChildRounding     = 10.0f;
    s.FrameRounding     = 6.0f;
    s.PopupRounding     = 8.0f;
    s.ScrollbarRounding = 8.0f;
    s.GrabRounding      = 6.0f;
    s.WindowBorderSize  = 0.0f;
    s.ChildBorderSize   = 1.0f;
    s.FrameBorderSize   = 1.0f;
    s.WindowPadding     = ImVec2(0.0f, 0.0f);
    s.FramePadding      = ImVec2(12.0f, 7.0f);
    s.ItemSpacing       = ImVec2(10.0f, 10.0f);
    s.ItemInnerSpacing  = ImVec2(8.0f, 6.0f);
    s.ScrollbarSize     = 14.0f;
    s.GrabMinSize       = 12.0f;
    s.ButtonTextAlign   = ImVec2(0.5f, 0.5f);
    s.SeparatorTextBorderSize = 2.0f;
    s.SeparatorTextAlign = ImVec2(0.0f, 0.5f);

    ImVec4 *c = s.Colors;

    c[ImGuiCol_WindowBg]        = g_pal.bg_top;
    c[ImGuiCol_ChildBg]         = g_pal.card;
    c[ImGuiCol_PopupBg]         = g_pal.card_header;
    c[ImGuiCol_Border]          = g_pal.edge;
    c[ImGuiCol_Text]            = g_pal.text;
    c[ImGuiCol_TextDisabled]    = g_pal.text_dim;
    c[ImGuiCol_FrameBg]         = g_pal.well;
    c[ImGuiCol_FrameBgHovered]  = ImVec4(0.10f, 0.14f, 0.28f, 1.0f);
    c[ImGuiCol_FrameBgActive]   = ImVec4(0.13f, 0.18f, 0.34f, 1.0f);
    c[ImGuiCol_TitleBg]         = g_pal.card_header;
    c[ImGuiCol_TitleBgActive]   = g_pal.card_header;
    c[ImGuiCol_Button]          = ImVec4(0.16f, 0.22f, 0.42f, 1.0f);
    c[ImGuiCol_ButtonHovered]   = ImVec4(0.22f, 0.30f, 0.54f, 1.0f);
    c[ImGuiCol_ButtonActive]    = g_pal.gold_dim;
    c[ImGuiCol_CheckMark]       = g_pal.gold_bright;
    c[ImGuiCol_SliderGrab]      = g_pal.gold;
    c[ImGuiCol_SliderGrabActive]= g_pal.gold_bright;
    c[ImGuiCol_Header]          = ImVec4(0.16f, 0.22f, 0.42f, 1.0f);
    c[ImGuiCol_HeaderHovered]   = ImVec4(0.22f, 0.30f, 0.54f, 1.0f);
    c[ImGuiCol_HeaderActive]    = g_pal.gold_dim;
    c[ImGuiCol_Separator]       = g_pal.edge;
    c[ImGuiCol_ScrollbarBg]     = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_ScrollbarGrab]   = ImVec4(0.22f, 0.30f, 0.54f, 1.0f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.38f, 0.66f, 1.0f);
    c[ImGuiCol_ScrollbarGrabActive]  = g_pal.gold_dim;
    c[ImGuiCol_PlotHistogram]   = g_pal.gold;
    c[ImGuiCol_TableHeaderBg]   = g_pal.card_header;
    c[ImGuiCol_TableBorderStrong] = g_pal.edge;
    c[ImGuiCol_TableBorderLight]  = ImVec4(0.14f, 0.19f, 0.36f, 1.0f);
    c[ImGuiCol_TableRowBg]      = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_TableRowBgAlt]   = ImVec4(1.0f, 1.0f, 1.0f, 0.03f);
    c[ImGuiCol_ModalWindowDimBg]= ImVec4(0.0f, 0.0f, 0.0f, 0.55f);
}

void Backdrop()
{
    ImVec2 p  = ImGui::GetWindowPos();
    ImVec2 sz = ImGui::GetWindowSize();
    ImGui::GetWindowDrawList()->AddRectFilledMultiColor(
        p, ImVec2(p.x + sz.x, p.y + sz.y),
        U32(g_pal.bg_top), U32(g_pal.bg_top),
        U32(g_pal.bg_bottom), U32(g_pal.bg_bottom));
}

void CardBegin(const char *id, const ImVec2 &size, const char *title)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_pal.card);
    ImGui::PushStyleColor(ImGuiCol_Border, g_pal.edge);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 14.0f));
    ImGui::BeginChild(id, size, true);

    ImVec2 p  = ImGui::GetWindowPos();
    ImVec2 sz = ImGui::GetWindowSize();
    const float hdr = 34.0f;

    ImDrawList *dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + sz.x, p.y + hdr), U32(g_pal.card_header),
                      10.0f, ImDrawFlags_RoundCornersTop);
    dl->AddRectFilled(ImVec2(p.x, p.y + hdr - 2.0f), ImVec2(p.x + sz.x, p.y + hdr),
                      U32(g_pal.gold));

    ImGui::SetCursorScreenPos(
        ImVec2(p.x + 16.0f, p.y + (hdr - ImGui::GetTextLineHeight()) * 0.5f));
    ImGui::PushStyleColor(ImGuiCol_Text, g_pal.gold_bright);
    ImGui::TextUnformatted(title);
    ImGui::PopStyleColor();

    ImGui::SetCursorScreenPos(ImVec2(p.x + 16.0f, p.y + hdr + 12.0f));
}

void CardEnd()
{
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

} /* namespace ui */
} /* namespace pit */
