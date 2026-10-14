/*
 * Visual theme for the PiT launcher.
 *
 * The palette is a Mario & Luigi flavoured take on the Nintendo DS: deep
 * cobalt panels, a cream body, and the series' gold star accent, with Mario red
 * and Luigi green reserved for state (errors and success). The layout motif is
 * the console itself - a wide top screen above a narrower bottom screen - which
 * the HOME view echoes with its two stacked cards.
 *
 * None of this reads the user's cartridge: it is plain launcher chrome, so
 * unlike the in-game UI there is no ROM provenance to record here.
 */

#ifndef PIT_UI_LAUNCHER_THEME_H
#define PIT_UI_LAUNCHER_THEME_H

#include <imgui.h>

namespace pit {
namespace ui {

struct Palette {
    ImVec4 bg_top;        /* window backdrop, top of the gradient */
    ImVec4 bg_bottom;     /* window backdrop, bottom of the gradient */
    ImVec4 sidebar;       /* navigation rail */
    ImVec4 sidebar_hi;    /* hovered / selected nav row */
    ImVec4 card;          /* panel body */
    ImVec4 card_header;   /* panel title strip */
    ImVec4 well;          /* inset inputs and log wells */
    ImVec4 edge;          /* hairline borders */

    ImVec4 gold;          /* primary accent (star) */
    ImVec4 gold_dim;
    ImVec4 gold_bright;

    ImVec4 text;          /* cream body text */
    ImVec4 text_dim;      /* secondary text */
    ImVec4 red;           /* errors */
    ImVec4 green;         /* success */
    ImVec4 amber;         /* warnings / busy */
};

extern Palette g_pal;

ImU32  U32(const ImVec4 &c);
ImU32  U32a(const ImVec4 &c, float alpha);

/* Applies colours and metrics. Call after ImGui::CreateContext(). */
void ApplyStyle();

/* Draw a gradient backdrop for the whole window (call inside a full-screen
 * window with no padding). */
void Backdrop();

/* A titled panel: fills a card background with a gold header strip. Pair every
 * CardBegin with CardEnd. */
void CardBegin(const char *id, const ImVec2 &size, const char *title);
void CardEnd();

} /* namespace ui */
} /* namespace pit */

#endif /* PIT_UI_LAUNCHER_THEME_H */
