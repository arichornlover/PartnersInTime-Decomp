/*
 * Partners in Time launcher - a C++ Dear ImGui front end.
 *
 * This is the standardised replacement for the hand-drawn SDL patcher. It calls
 * the same pit_core the headless driver uses, so verify, prepare and export
 * behave identically no matter which front end runs them.
 *
 * The look borrows the console itself: a cobalt rail, a cream body and the
 * series' gold star accent, with the HOME view laid out as a wide "top screen"
 * source card above a narrower "bottom screen" output card. Nothing here reads
 * the cartridge for its own sake; the only ROM access is through pit_core while
 * the user runs a job.
 *
 * Patching runs on a worker thread. Its progress callback is the same one the
 * headless driver and the old SDL build use; the only addition is a mutex,
 * because the callback arrives from the worker while the main loop repaints.
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif

#include <SDL.h>

#ifdef _WIN32
#include <SDL_syswm.h>
#endif

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#endif

extern "C" {
#include "core/pit_ingest.h"
#include "core/pit_mods.h"
#include "core/pit_patch_data.h"
#include "core/pit_patcher.h"
}

#include "ui/launcher_config.hpp"
#include "ui/launcher_theme.hpp"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#endif

static const char *k_version = "PiT Launcher 0.1";

using pit::ui::Config;

/* --------------------------------------------------------------- mod list */

struct ModEntry {
    std::string id;
    std::string name;
    std::string version;
    std::string desc;
    std::string applies;
    std::string summary;
    const pit_mod_profile *profile = nullptr;
};

struct LogLine {
    int level; /* 0 info, 1 ok, 2 warn, 3 error */
    std::string text;
};

struct App {
    Config cfg;
    std::string cfg_path;

    char src_buf[600];
    char out_buf[600];
    char mods_buf[600];

    std::vector<ModEntry> mods;
    pit_mod_catalog catalog{};
    int selected = 0;
    bool auto_output = true;

    std::vector<LogLine> log;
    std::mutex log_mutex;

    std::atomic<bool> busy{false};
    std::atomic<int> step{0};
    std::atomic<double> fraction{0.0};
    int last_result = -1;
    pit_patch_info info{};
    std::string ingest_note;

    std::thread worker;
    bool worker_running = false;

    int tab = 0; /* 0 home, 1 mods, 2 settings, 3 about */
    bool request_quit = false;
    std::string toast;
    double toast_until = 0.0;
};

/* ImGui fonts, created once in main(). */
static ImFont *g_font_body = nullptr;
static ImFont *g_font_title = nullptr;

#ifdef _WIN32
static HWND g_hwnd = nullptr;
#endif

/* --------------------------------------------------------------- logging */

static void LogAdd(App *app, int level, const std::string &text)
{
    std::lock_guard<std::mutex> lk(app->log_mutex);
    if (app->log.size() < 800) {
        app->log.push_back({level, text});
    }
}

static void PatchLog(void *ctx, int step, const char *message, double fraction)
{
    App *app = static_cast<App *>(ctx);
    app->step.store(step);
    app->fraction.store(fraction);
    if (message && message[0] != '\0') {
        LogAdd(app, 0, message);
    }
}

/* ------------------------------------------------------------ path helpers */

static std::string DirOf(const std::string &path)
{
    size_t sep = path.find_last_of("/\\");
    if (sep == std::string::npos) {
        return std::string();
    }
    return path.substr(0, sep);
}

static std::string ExportDirFor(const std::string &output_path)
{
    std::string dir = DirOf(output_path);
    if (dir.empty()) {
        return "generated";
    }
    return dir + "\\generated";
}

static std::string SafeId(const std::string &id)
{
    std::string out;
    for (char ch : id) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') || ch == '_' || ch == '-') {
            out.push_back(ch);
        }
        if (out.size() >= 24) {
            break;
        }
    }
    return out;
}

static std::string SuffixFor(const App &app, int idx)
{
    if (idx <= 0) {
        return ".prepared.nds";
    }
    std::string id = SafeId(app.mods[idx].id);
    if (id.empty()) {
        return ".prepared.nds";
    }
    return "." + id + ".nds";
}

static void DeriveOutput(App *app)
{
    if (!app->auto_output) {
        return;
    }
    std::string src = app->src_buf;
    if (src.empty()) {
        app->out_buf[0] = '\0';
        return;
    }
    std::string out = src + SuffixFor(*app, app->selected);
    std::snprintf(app->out_buf, sizeof(app->out_buf), "%s", out.c_str());
}

/* --------------------------------------------------------------- mod scan */

/*
 * Hard Mode is an ordinary mod: a folder called hard_mode holding a
 * profile.json. The release ships one from port/mods, and the launcher also
 * recreates it on first run so a user who clears the folder still starts with a
 * working default. The embedded copy mirrors port/mods/hard_mode/profile.json;
 * keep the two in step.
 */
static const char *k_hard_mode_profile = R"PITPROFILE({
  "schema": "pit-mod-profile-v1",
  "id": "hard_mode",
  "name": "Hard Mode",
  "version": "1.0.0",
  "license": "CC-BY-4.0",
  "authors": [],
  "description": "Raises enemy durability and offence for a real difficulty spike, while rewards keep the loop from dragging.",
  "applies_to": {
    "version": "eur",
    "stat_document": "stats/enemies.json"
  },
  "transforms": [
    { "field": "max_hp", "scale": 1.1, "min": 1, "max": 65535, "enabled": true },
    { "field": "power", "scale": 2.0, "min": 0, "max": 65535, "enabled": true },
    { "field": "defense", "scale": 1.05, "min": 1, "max": 65535, "enabled": true },
    { "field": "speed", "scale": 1.25, "min": 1, "max": 65535, "enabled": true },
    { "field": "coins", "scale": 1.75, "min": 0, "max": 65535, "enabled": true },
    { "field": "experience", "scale": 1.75, "min": 0, "max": 65535, "enabled": true }
  ]
})PITPROFILE";

static const char *k_mods_readme =
    "Mods\r\n"
    "====\r\n"
    "\r\n"
    "Each subfolder here is one mod: <id>/profile.json.\r\n"
    "\r\n"
    "A profile only scales the numeric fields of the stat record it names; it\r\n"
    "never carries ROM data. The launcher lists every valid profile it finds in\r\n"
    "this folder, so adding a mod is dropping in a folder.\r\n";

static bool FileExists(const std::string &path)
{
    FILE *fp = std::fopen(path.c_str(), "rb");
    if (fp) {
        std::fclose(fp);
        return true;
    }
    return false;
}

static void EnsureDir(const std::string &path)
{
#ifdef _WIN32
    _mkdir(path.c_str());
#else
    mkdir(path.c_str(), 0755);
#endif
}

static void WriteTextFile(const std::string &path, const char *text)
{
    FILE *fp = std::fopen(path.c_str(), "wb");
    if (!fp) {
        return;
    }
    std::fwrite(text, 1, std::strlen(text), fp);
    std::fclose(fp);
}

/* Default mods folder: a "mods" folder beside the launcher executable. */
static std::string DefaultModsDir()
{
    char *base = SDL_GetBasePath();
    if (!base) {
        return "mods";
    }
    std::string dir = base;
    SDL_free(base);
    while (!dir.empty() && (dir.back() == '/' || dir.back() == '\\')) {
        dir.pop_back();
    }
    if (dir.empty()) {
        return "mods";
    }
    return dir + "/mods";
}

static void SeedModsDir(const std::string &dir)
{
    if (dir.empty()) {
        return;
    }
    EnsureDir(dir);
    if (!FileExists(dir + "/README.txt")) {
        WriteTextFile(dir + "/README.txt", k_mods_readme);
    }

    std::string hm = dir + "/hard_mode";
    EnsureDir(hm);
    if (!FileExists(hm + "/profile.json")) {
        WriteTextFile(hm + "/profile.json", k_hard_mode_profile);
    }
}

static void BuildModList(App *app)
{
    app->mods.clear();

    ModEntry none;
    none.name = "No mod";
    none.desc = "Prepare a clean, header-repaired copy of your own cartridge.";
    none.applies = PIT_PLAN_ROM_BADGE;
    none.summary = "no changes";
    app->mods.push_back(none);

    char err[256] = {0};
    unsigned int skipped = 0;
    if (pit_mods_scan(app->cfg.mods_dir.c_str(), &app->catalog, err, sizeof(err),
                      &skipped) >= 0) {
        for (unsigned int i = 0; i < app->catalog.count; i++) {
            ModEntry e;
            e.id = app->catalog.profiles[i].id;
            e.name = app->catalog.profiles[i].name;
            e.version = app->catalog.profiles[i].version;
            e.desc = app->catalog.profiles[i].description;
            e.applies = app->catalog.profiles[i].applies_to;
            e.profile = &app->catalog.profiles[i];
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%u transforms",
                          app->catalog.profiles[i].transform_count);
            e.summary = buf;
            app->mods.push_back(e);
        }
    }

    app->selected = 0;
    if (!app->cfg.mod_id.empty()) {
        for (size_t i = 0; i < app->mods.size(); i++) {
            if (app->mods[i].id == app->cfg.mod_id) {
                app->selected = static_cast<int>(i);
                break;
            }
        }
    }
}

/* --------------------------------------------------------------- pipeline */

static void RunPipeline(App *app)
{
    std::string src = app->src_buf;
    std::string out = app->out_buf;
    const bool export_assets = app->cfg.export_assets;

    int apply = app->selected >= 1 ? 1 : 0;
    const pit_mod_profile *profile = nullptr;
    if (app->selected >= 1 && app->selected < static_cast<int>(app->mods.size())) {
        profile = app->mods[app->selected].profile;
    }

    app->info = pit_patch_info{};
    pit_patch_result r = pit_patcher_run_ex(src.c_str(), out.c_str(), apply, profile,
                                            PatchLog, app, &app->info);
    app->last_result = static_cast<int>(r);

    if (r == PIT_PATCH_OK && export_assets) {
        std::string dir = ExportDirFor(out);
        LogAdd(app, 0, "Exporting generated data to " + dir);
        if (pit_ingest_run(src.c_str(), dir.c_str(), &app->info, PatchLog, app) != 0) {
            app->ingest_note = "asset export failed";
        }
    }

    app->busy.store(false);
}

static void StartRun(App *app)
{
    if (app->busy.load()) {
        return;
    }
    if (app->src_buf[0] == '\0') {
        LogAdd(app, 3, "Choose your source ROM first.");
        return;
    }
    if (app->out_buf[0] == '\0') {
        DeriveOutput(app);
    }
    if (app->out_buf[0] == '\0') {
        LogAdd(app, 3, "Choose where to write the prepared copy.");
        return;
    }

    {
        std::lock_guard<std::mutex> lk(app->log_mutex);
        app->log.clear();
    }
    app->ingest_note.clear();
    app->last_result = -1;
    app->step.store(0);
    app->fraction.store(0.0);
    app->busy.store(true);

    LogAdd(app, 0, "Your EUR ROM is the only data source.");
    LogAdd(app, 0, "Source: " + std::string(app->src_buf));
    LogAdd(app, 0, "Output: " + std::string(app->out_buf));

    app->worker = std::thread(RunPipeline, app);
    app->worker_running = true;
}

static void FinishRun(App *app)
{
    if (app->worker.joinable()) {
        app->worker.join();
    }
    app->worker_running = false;

    if (app->last_result == PIT_PATCH_OK) {
        app->toast = app->ingest_note.empty() ? "Prepared copy written."
                                              : "Prepared copy written; export failed.";
        app->toast_until = ImGui::GetTime() + 4.0;
        LogAdd(app, 1, "Done. " + std::string(pit_patch_result_text(
                            static_cast<pit_patch_result>(app->last_result))));
    } else if (app->last_result >= 0) {
        app->toast = "Failed: " + std::string(pit_patch_result_text(
                          static_cast<pit_patch_result>(app->last_result)));
        app->toast_until = ImGui::GetTime() + 5.0;
    }
}

/* --------------------------------------------------------------- dialogs */

static bool FileDialog(std::string &path, bool save, const char *title)
{
#ifdef _WIN32
    char buf[1024];
    std::snprintf(buf, sizeof(buf), "%s", path.c_str());

    static const char filter[] =
        "Nintendo DS ROM (*.nds;*.dsi)\0*.nds;*.dsi\0"
        "All files (*.*)\0*.*\0\0";

    OPENFILENAMEA ofn;
    std::memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = buf;
    ofn.nMaxFile = sizeof(buf);
    ofn.lpstrTitle = title;
    ofn.lpstrDefExt = "nds";
    ofn.Flags = OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST |
                (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);

    BOOL ok = save ? GetSaveFileNameA(&ofn) : GetOpenFileNameA(&ofn);
    if (ok) {
        path = buf;
        return true;
    }
#else
    (void)path;
    (void)save;
    (void)title;
#endif
    return false;
}

static void OpenFolder(const std::string &dir)
{
#ifdef _WIN32
    if (!dir.empty()) {
        ShellExecuteA(nullptr, "open", dir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
#else
    (void)dir;
#endif
}

/* --------------------------------------------------------------- widgets */

static void HelpMarker(const char *text)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

static bool GoldButton(const char *label, const ImVec2 &size, bool enabled = true)
{
    using namespace pit::ui;
    if (!enabled) {
        ImGui::BeginDisabled(true);
        ImGui::Button(label, size);
        ImGui::EndDisabled();
        return false;
    }
    ImGui::PushStyleColor(ImGuiCol_Button, U32(g_pal.gold));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, U32(g_pal.gold_bright));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, U32(g_pal.gold_dim));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.13f, 0.09f, 0.02f, 1.0f));
    bool pressed = ImGui::Button(label, size);
    ImGui::PopStyleColor(4);
    return pressed;
}

static void SegmentedProgress(float fraction, int step, int total)
{
    using namespace pit::ui;
    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    const float h = 14.0f;
    const float gap = 4.0f;
    if (total < 1) {
        total = 1;
    }
    float seg_w = (w - gap * (total - 1)) / total;

    for (int i = 0; i < total; i++) {
        float x0 = p.x + i * (seg_w + gap);
        float x1 = x0 + seg_w;
        ImVec4 base = ImVec4(0.09f, 0.12f, 0.24f, 1.0f);
        ImU32 col = U32(base);
        if (i < step - 1 || (i == step - 1 && fraction >= 1.0f)) {
            col = U32(g_pal.green);
        } else if (i == step - 1) {
            col = U32(g_pal.gold);
        }
        dl->AddRectFilled(ImVec2(x0, p.y), ImVec2(x1, p.y + h), col, 3.0f);
    }
    ImGui::Dummy(ImVec2(w, h));
}

/* --------------------------------------------------------------- sidebar */

static void DrawSidebar(App *app)
{
    using namespace pit::ui;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_pal.sidebar);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(22.0f, 20.0f));
    ImGui::BeginChild("##sidebar", ImVec2(240.0f, 0.0f), false);

    /* Wordmark: the two brothers' colours over the series' gold. */
    ImGui::PushFont(g_font_title);
    ImGui::PushStyleColor(ImGuiCol_Text, g_pal.red);
    ImGui::TextUnformatted("MARIO");
    ImGui::PopStyleColor();
    ImGui::SameLine(0.0f, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::TextUnformatted("&");
    ImGui::PopStyleColor();
    ImGui::SameLine(0.0f, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, g_pal.green);
    ImGui::TextUnformatted("LUIGI");
    ImGui::PopStyleColor();
    ImGui::PopFont();

    ImGui::PushStyleColor(ImGuiCol_Text, g_pal.gold);
    ImGui::TextUnformatted("PARTNERS IN TIME");
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Text, g_pal.text_dim);
    ImGui::TextUnformatted("LAUNCHER");
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0.0f, 14.0f));
    ImGui::PushStyleColor(ImGuiCol_Separator, g_pal.edge);
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0.0f, 10.0f));

    static const char *items[] = {"HOME", "MODS", "SETTINGS", "ABOUT"};
    for (int i = 0; i < 4; i++) {
        bool active = (app->tab == i);
        ImVec2 p = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;
        const float h = 40.0f;

        if (active) {
            ImGui::GetWindowDrawList()->AddRectFilled(
                p, ImVec2(p.x + w, p.y + h), U32(g_pal.sidebar_hi), 8.0f);
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(p.x, p.y + 6.0f), ImVec2(p.x + 4.0f, p.y + h - 6.0f),
                U32(g_pal.gold), 2.0f);
        }

        ImGui::PushStyleColor(ImGuiCol_Header,
                              active ? ImVec4(0, 0, 0, 0) : ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, U32a(g_pal.sidebar_hi, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, U32(g_pal.sidebar_hi));
        ImGui::PushStyleColor(ImGuiCol_Text, active ? g_pal.gold_bright : g_pal.text);
        if (ImGui::Selectable(items[i], active, 0, ImVec2(w, h))) {
            app->tab = i;
        }
        ImGui::PopStyleColor(4);
        ImGui::Dummy(ImVec2(0.0f, 2.0f));
    }

    /* Footer pinned to the bottom. */
    float footer = ImGui::GetWindowHeight() - 92.0f;
    if (footer > ImGui::GetCursorPosY()) {
        ImGui::SetCursorPosY(footer);
    }
    ImGui::PushStyleColor(ImGuiCol_Text, g_pal.text_dim);
    ImGui::TextUnformatted(k_version);
    ImGui::TextUnformatted("EUR cartridge required");
    ImGui::PopStyleColor();

    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

/* --------------------------------------------------------------- home */

static void ViewHome(App *app)
{
    using namespace pit::ui;

    ImGui::PushFont(g_font_title);
    ImGui::PushStyleColor(ImGuiCol_Text, g_pal.text);
    ImGui::TextUnformatted("Prepare your cartridge");
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::PushStyleColor(ImGuiCol_Text, g_pal.text_dim);
    ImGui::TextWrapped("Reads your own legally obtained ROM, verifies it, and "
                       "writes a prepared copy. No Nintendo data is shipped.");
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0.0f, 6.0f));

    float avail_w = ImGui::GetContentRegionAvail().x;
    float card_w = avail_w;

    /* Top screen: source cartridge. */
    CardBegin("##card_src", ImVec2(card_w, 132.0f), "SOURCE  -  TOP SCREEN");
    ImGui::PushStyleColor(ImGuiCol_Text, g_pal.text_dim);
    ImGui::TextUnformatted("Your Partners in Time (EUR) cartridge");
    ImGui::PopStyleColor();
    ImGui::SetNextItemWidth(card_w - 130.0f);
    ImGui::InputText("##src", app->src_buf, sizeof(app->src_buf));
    ImGui::SameLine();
    if (ImGui::Button("Browse...", ImVec2(110.0f, 0.0f))) {
        std::string p = app->src_buf;
        if (FileDialog(p, false, "Choose your source ROM")) {
            std::snprintf(app->src_buf, sizeof(app->src_buf), "%s", p.c_str());
            DeriveOutput(app);
        }
    }
    CardEnd();
    ImGui::Dummy(ImVec2(0.0f, 10.0f));

    /* Bottom screen: output + mod + options. */
    CardBegin("##card_out", ImVec2(card_w, 176.0f), "PREPARED COPY  -  BOTTOM SCREEN");
    ImGui::PushStyleColor(ImGuiCol_Text, g_pal.text_dim);
    ImGui::TextUnformatted("Where the patched copy is written");
    ImGui::PopStyleColor();
    ImGui::SetNextItemWidth(card_w - 130.0f);
    if (ImGui::InputText("##out", app->out_buf, sizeof(app->out_buf))) {
        app->auto_output = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Save as...", ImVec2(110.0f, 0.0f))) {
        std::string p = app->out_buf;
        if (FileDialog(p, true, "Save prepared copy")) {
            std::snprintf(app->out_buf, sizeof(app->out_buf), "%s", p.c_str());
            app->auto_output = false;
        }
    }

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    ImGui::TextUnformatted("Mod");
    ImGui::SameLine();
    const char *mod_name =
        (app->selected >= 0 && app->selected < static_cast<int>(app->mods.size()))
            ? app->mods[app->selected].name.c_str()
            : "No mod";
    ImGui::PushStyleColor(ImGuiCol_Text, g_pal.gold_bright);
    ImGui::TextUnformatted(mod_name);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    HelpMarker("Choose a profile on the MODS tab. The bundled HARD MODE mod and "
               "any folder under mods/ are listed there.");
    ImGui::SameLine();
    if (ImGui::Checkbox("Auto-name", &app->auto_output)) {
        DeriveOutput(app);
    }
    ImGui::SameLine();
    ImGui::Checkbox("Export data", &app->cfg.export_assets);

    CardEnd();

    ImGui::Dummy(ImVec2(0.0f, 12.0f));

    /* Action row. */
    float run_w = 190.0f;
    bool can_run = !app->busy.load();
    if (GoldButton(app->busy.load() ? "Working..." : "PREPARE ROM",
                   ImVec2(run_w, 40.0f), can_run)) {
        StartRun(app);
    }
    ImGui::SameLine();
    ImGui::BeginGroup();
    int total = PIT_PATCH_STEPS;
    int step = app->step.load();
    float frac = static_cast<float>(app->fraction.load());
    SegmentedProgress(frac, step, total);
    ImGui::PushStyleColor(ImGuiCol_Text, app->busy.load() ? g_pal.amber : g_pal.text_dim);
    if (app->busy.load()) {
        ImGui::Text("Step %d of %d  -  %d%%", step, total, (int)(frac * 100.0f + 0.5f));
    } else if (app->last_result == PIT_PATCH_OK) {
        ImGui::PushStyleColor(ImGuiCol_Text, g_pal.green);
        ImGui::TextUnformatted("Ready.");
    } else {
        ImGui::TextUnformatted("Idle.");
    }
    if (app->busy.load() || app->last_result == PIT_PATCH_OK) {
        ImGui::PopStyleColor();
    }
    ImGui::PopStyleColor();
    ImGui::EndGroup();

    if (!app->toast.empty() && ImGui::GetTime() < app->toast_until) {
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        bool good = app->last_result == PIT_PATCH_OK;
        ImGui::PushStyleColor(ImGuiCol_Text, good ? g_pal.green : g_pal.red);
        ImGui::TextUnformatted(app->toast.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::Dummy(ImVec2(0.0f, 10.0f));

    /* Log. */
    if (ImGui::CollapsingHeader("Log", app->cfg.show_log ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, g_pal.well);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
        ImGui::BeginChild("##log", ImVec2(0.0f, 170.0f), true);
        std::lock_guard<std::mutex> lk(app->log_mutex);
        for (const LogLine &l : app->log) {
            ImVec4 col = g_pal.text;
            if (l.level == 1) col = g_pal.green;
            else if (l.level == 2) col = g_pal.amber;
            else if (l.level == 3) col = g_pal.red;
            ImGui::PushStyleColor(ImGuiCol_Text, col);
            ImGui::TextWrapped("%s", l.text.c_str());
            ImGui::PopStyleColor();
        }
        if (app->log.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, g_pal.text_dim);
            ImGui::TextUnformatted("Nothing yet.");
            ImGui::PopStyleColor();
        }
        if (app->busy.load() &&
            ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 4.0f) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }

    if (app->last_result == PIT_PATCH_OK) {
        ImGui::Dummy(ImVec2(0.0f, 6.0f));
        if (ImGui::Button("Open output folder", ImVec2(180.0f, 0.0f))) {
            OpenFolder(DirOf(app->out_buf));
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, g_pal.text_dim);
        ImGui::Text("records patched: %u   fields: %u",
                    app->info.records_patched, app->info.fields_written);
        ImGui::PopStyleColor();
    }
}

/* --------------------------------------------------------------- mods */

static void ViewMods(App *app)
{
    using namespace pit::ui;

    ImGui::PushFont(g_font_title);
    ImGui::PushStyleColor(ImGuiCol_Text, g_pal.text);
    ImGui::TextUnformatted("Mod profiles");
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::PushStyleColor(ImGuiCol_Text, g_pal.text_dim);
    ImGui::TextWrapped("A profile only scales fields; it never carries ROM data. "
                       "Drop a mods/<id>/profile.json folder beside the launcher.");
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0.0f, 6.0f));

    float w = ImGui::GetContentRegionAvail().x;

    CardBegin("##card_mods", ImVec2(w * 0.5f - 8.0f, 320.0f), "AVAILABLE");
    for (size_t i = 0; i < app->mods.size(); i++) {
        const ModEntry &m = app->mods[i];
        ImGui::PushID(static_cast<int>(i));
        bool is_sel = (app->selected == static_cast<int>(i));
        if (ImGui::RadioButton("", is_sel)) {
            app->selected = static_cast<int>(i);
            DeriveOutput(app);
        }
        ImGui::SameLine();
        ImVec4 col = is_sel ? g_pal.gold_bright : g_pal.text;
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::TextUnformatted(m.name.c_str());
        ImGui::PopStyleColor();
        if (!m.version.empty()) {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, g_pal.text_dim);
            ImGui::Text("v%s", m.version.c_str());
            ImGui::PopStyleColor();
        }
        ImGui::PopID();
    }
    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    CardEnd();

    ImGui::SameLine();

    CardBegin("##card_desc", ImVec2(0.0f, 320.0f), "DETAILS");
    if (app->selected >= 0 && app->selected < static_cast<int>(app->mods.size())) {
        const ModEntry &m = app->mods[app->selected];
        ImGui::PushStyleColor(ImGuiCol_Text, g_pal.gold_bright);
        ImGui::TextUnformatted(m.name.c_str());
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Text, g_pal.text_dim);
        ImGui::TextWrapped("%s", m.desc.c_str());
        ImGui::Text("Applies to: %s", m.applies.empty() ? "any" : m.applies.c_str());
        ImGui::Text("%s", m.summary.c_str());
        ImGui::PopStyleColor();
    }
    CardEnd();

    ImGui::Dummy(ImVec2(0.0f, 12.0f));
    ImGui::TextUnformatted("Folder");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 240.0f);
    ImGui::InputText("##modsdir", app->mods_buf, sizeof(app->mods_buf));
    ImGui::SameLine();
    if (ImGui::Button("Rescan", ImVec2(110.0f, 0.0f))) {
        app->cfg.mods_dir = app->mods_buf;
        SeedModsDir(app->mods_buf);
        BuildModList(app);
        DeriveOutput(app);
    }
    ImGui::SameLine();
    if (ImGui::Button("Open", ImVec2(110.0f, 0.0f))) {
        OpenFolder(app->mods_buf);
    }
}

/* --------------------------------------------------------------- settings */

static void ViewSettings(App *app)
{
    using namespace pit::ui;

    ImGui::PushFont(g_font_title);
    ImGui::PushStyleColor(ImGuiCol_Text, g_pal.text);
    ImGui::TextUnformatted("Settings");
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(0.0f, 6.0f));

    float w = ImGui::GetContentRegionAvail().x;
    CardBegin("##card_set", ImVec2(w, 320.0f), "PREFERENCES");

    ImGui::Checkbox("Export generated data after preparing", &app->cfg.export_assets);
    ImGui::SameLine();
    HelpMarker("Writes the banner PNG, file tree and manifest the decompilation "
               "reads, beside the prepared copy.");
    ImGui::Checkbox("Vertical sync", &app->cfg.vsync);
    ImGui::Checkbox("Show log panel", &app->cfg.show_log);
    ImGui::Dummy(ImVec2(0.0f, 6.0f));

    ImGui::TextUnformatted("Interface scale");
    ImGui::SetNextItemWidth(240.0f);
    ImGui::SliderFloat("##scale", &app->cfg.ui_scale, 0.8f, 1.5f, "%.2fx");

    ImGui::Dummy(ImVec2(0.0f, 6.0f));
    ImGui::TextUnformatted("Mods folder");
    ImGui::SetNextItemWidth(w - 40.0f);
    ImGui::InputText("##smods", app->mods_buf, sizeof(app->mods_buf));

    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    if (GoldButton("Save settings", ImVec2(180.0f, 36.0f))) {
        app->cfg.mods_dir = app->mods_buf;
        SeedModsDir(app->mods_buf);
        if (app->selected >= 0 && app->selected < static_cast<int>(app->mods.size())) {
            app->cfg.mod_id = app->mods[app->selected].id;
        }
        app->cfg.source_rom = app->src_buf;
        app->cfg.output_rom = app->out_buf;
        if (pit::ui::ConfigSave(app->cfg, app->cfg_path)) {
            app->toast = "Settings saved.";
            app->toast_until = ImGui::GetTime() + 3.0;
        } else {
            app->toast = "Could not save settings.";
            app->toast_until = ImGui::GetTime() + 3.0;
        }
        BuildModList(app);
    }
    ImGui::SameLine();
    if (ImGui::Button("Open settings folder", ImVec2(200.0f, 36.0f))) {
        OpenFolder(DirOf(app->cfg_path));
    }

    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, g_pal.text_dim);
    ImGui::TextWrapped("Settings file: %s", app->cfg_path.c_str());
    ImGui::PopStyleColor();
    CardEnd();
}

/* --------------------------------------------------------------- about */

static void ViewAbout(App *app)
{
    using namespace pit::ui;
    (void)app;

    ImGui::PushFont(g_font_title);
    ImGui::PushStyleColor(ImGuiCol_Text, g_pal.text);
    ImGui::TextUnformatted("About");
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(0.0f, 6.0f));

    float w = ImGui::GetContentRegionAvail().x;
    CardBegin("##card_about", ImVec2(w, 360.0f), "PARTNERS IN TIME LAUNCHER");

    ImGui::PushStyleColor(ImGuiCol_Text, g_pal.gold_bright);
    ImGui::TextUnformatted(k_version);
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0.0f, 4.0f));

    ImGui::TextWrapped(
        "This launcher prepares a copy of your own Mario & Luigi: Partners in "
        "Time cartridge for the community decompilation project. It verifies the "
        "ROM, repairs the header checksum, optionally applies a balance mod, and "
        "can export the generated data the project reads.");

    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    ImGui::SeparatorText("Legal");
    ImGui::TextWrapped(
        "Contains no Nintendo assets. Graphics, audio and text remain the "
        "property of Nintendo and are read only from your own ROM. You must "
        "supply a legally obtained copy.");

    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    ImGui::SeparatorText("Supported cartridge");
    ImGui::BulletText("EUR  -  SHA-1 ba4ec2f9...e28701  (ARMP, 64 MiB)");
    ImGui::PushStyleColor(ImGuiCol_Text, g_pal.text_dim);
    ImGui::TextWrapped("USA and USA Rev 1 cartridges are recognised but the "
                       "shipped mods target the EUR release.");
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    ImGui::SeparatorText("Built with");
        ImGui::BulletText("Dear ImGui  -  SDL2  -  SDL_Renderer");
    CardEnd();
}

/* --------------------------------------------------------------- root */

static void DrawUI(App *app)
{
    using namespace pit::ui;
    ImGuiIO &io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("##root", nullptr, flags);
    Backdrop();

    DrawSidebar(app);
    ImGui::SameLine();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(28.0f, 24.0f));
    ImGui::BeginChild("##content", ImVec2(0.0f, 0.0f), false,
                      ImGuiWindowFlags_NoBackground);
    switch (app->tab) {
        case 0: ViewHome(app); break;
        case 1: ViewMods(app); break;
        case 2: ViewSettings(app); break;
        default: ViewAbout(app); break;
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();

    ImGui::End();
    ImGui::PopStyleVar();
}

/* --------------------------------------------------------------- screenshot */

static unsigned char *ReadPixels(SDL_Renderer *renderer, int w, int h)
{
    unsigned char *raw = (unsigned char *)std::malloc((size_t)w * h * 4);
    if (!raw) {
        return nullptr;
    }
    if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_ARGB8888, raw,
                             w * 4) != 0) {
        std::free(raw);
        return nullptr;
    }
    return raw;
}

static bool WriteBMP(const char *path, int w, int h, const unsigned char *argb)
{
    int row = (w * 3 + 3) & ~3;
    unsigned int img = (unsigned int)(row * h);
    unsigned int file = 54u + img;
    unsigned char hdr[54];
    std::memset(hdr, 0, sizeof(hdr));
    hdr[0] = 'B'; hdr[1] = 'M';
    hdr[2] = (unsigned char)(file); hdr[3] = (unsigned char)(file >> 8);
    hdr[4] = (unsigned char)(file >> 16); hdr[5] = (unsigned char)(file >> 24);
    hdr[10] = 54;
    hdr[14] = 40;
    hdr[18] = (unsigned char)(w); hdr[19] = (unsigned char)(w >> 8);
    hdr[20] = (unsigned char)(w >> 16); hdr[21] = (unsigned char)(w >> 24);
    hdr[22] = (unsigned char)(h); hdr[23] = (unsigned char)(h >> 8);
    hdr[24] = (unsigned char)(h >> 16); hdr[25] = (unsigned char)(h >> 24);
    hdr[26] = 1; hdr[28] = 24;

    FILE *f = std::fopen(path, "wb");
    if (!f) {
        return false;
    }
    std::fwrite(hdr, 1, sizeof(hdr), f);
    std::vector<unsigned char> line((size_t)row, 0);
    /* SDL_RenderReadPixels returns rows top-to-bottom; BMP is bottom-up with a
     * positive height, so walk the source rows in reverse. ARGB8888 is
     * 0xAARRGGBB little-endian, i.e. B, G, R, A in memory: BMP's channel
     * order. */
    for (int y = 0; y < h; y++) {
        const unsigned char *src = argb + (size_t)(h - 1 - y) * w * 4;
        for (int x = 0; x < w; x++) {
            line[(size_t)x * 3 + 0] = src[(size_t)x * 4 + 0];
            line[(size_t)x * 3 + 1] = src[(size_t)x * 4 + 1];
            line[(size_t)x * 3 + 2] = src[(size_t)x * 4 + 2];
        }
        std::fwrite(line.data(), 1, (size_t)row, f);
    }
    std::fclose(f);
    return true;
}

/* --------------------------------------------------------------- main */

static void PrintUsage(void)
{
    std::printf(
        "%s\n"
        "Usage: pit_launcher [options] [source.nds] [output.nds]\n"
        "  --mods-dir <dir>       folder scanned for mod profiles (default: mods\n"
        "                         beside the launcher executable)\n"
        "  --hidden               do not show the window (for checks)\n"
        "  --frames <n>           render n frames then exit (for checks)\n"
        "  --screenshot <file>    write a .bmp of the last frame\n"
        "  --tab <0-3>            start on a view (home/mods/settings/about)\n"
        "  -h, --help             this text\n", k_version);
}

int main(int argc, char **argv)
{
    std::string screenshot;
    int frame_limit = 0;
    bool hidden = false;
    int positional = 0;

    App app;
    app.src_buf[0] = '\0';
    app.out_buf[0] = '\0';
    app.mods_buf[0] = '\0';

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            PrintUsage();
            return 0;
        } else if (arg == "--hidden") {
            hidden = true;
        } else if (arg == "--frames" && i + 1 < argc) {
            frame_limit = std::atoi(argv[++i]);
        } else if (arg == "--screenshot" && i + 1 < argc) {
            screenshot = argv[++i];
        } else if (arg == "--mods-dir" && i + 1 < argc) {
            std::snprintf(app.mods_buf, sizeof(app.mods_buf), "%s", argv[++i]);
        } else if (arg == "--tab" && i + 1 < argc) {
            app.tab = std::atoi(argv[++i]);
            if (app.tab < 0 || app.tab > 3) {
                app.tab = 0;
            }
        } else if (!arg.empty() && arg[0] != '-') {
            if (positional == 0) {
                std::snprintf(app.src_buf, sizeof(app.src_buf), "%s", argv[i]);
                positional++;
            } else if (positional == 1) {
                std::snprintf(app.out_buf, sizeof(app.out_buf), "%s", argv[i]);
                app.auto_output = false;
                positional++;
            }
        }
    }

    /* Settings. Command-line paths win over the stored ones. */
    app.cfg_path = pit::ui::ConfigPath();
    pit::ui::ConfigLoad(app.cfg, app.cfg_path);
    if (app.mods_buf[0] == '\0') {
        std::string dir = app.cfg.mods_dir.empty() ? DefaultModsDir()
                                                   : app.cfg.mods_dir;
        std::snprintf(app.mods_buf, sizeof(app.mods_buf), "%s", dir.c_str());
    }
    /* Generate the mods folder (with Hard Mode) beside the launcher if absent. */
    SeedModsDir(app.mods_buf);
    if (app.src_buf[0] == '\0' && !app.cfg.source_rom.empty()) {
        std::snprintf(app.src_buf, sizeof(app.src_buf), "%s", app.cfg.source_rom.c_str());
    }
    if (app.out_buf[0] == '\0' && !app.cfg.output_rom.empty()) {
        std::snprintf(app.out_buf, sizeof(app.out_buf), "%s", app.cfg.output_rom.c_str());
    }
    BuildModList(&app);
    if (app.out_buf[0] == '\0') {
        DeriveOutput(&app);
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    Uint32 win_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    if (hidden) {
        win_flags |= SDL_WINDOW_HIDDEN;
    }

    SDL_Window *window = SDL_CreateWindow("Partners in Time Launcher",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED, 1200, 760,
                                          win_flags);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    /* SDL_Renderer keeps the launcher off any specific GL version: the
     * accelerated path is used when a driver is present and the software
     * renderer otherwise, which is what lets the headless frame checks run on a
     * GPU-less CI runner. */
    SDL_Renderer *renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_RenderSetVSync(renderer, app.cfg.vsync ? 1 : 0);

#ifdef _WIN32
    {
        SDL_SysWMinfo info;
        SDL_VERSION(&info.version);
        if (SDL_GetWindowWMInfo(window, &info)) {
            g_hwnd = info.info.win.window;
        }
    }
#endif

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    /* Prefer a real Windows UI font for a modern look; fall back to the
     * built-in bitmap font when a system face is unavailable. */
#ifdef _WIN32
    const char *k_body_font  = "C:\\Windows\\Fonts\\segoeui.ttf";
    const char *k_title_font = "C:\\Windows\\Fonts\\segoeuib.ttf";
    const bool have_body  = GetFileAttributesA(k_body_font)  != INVALID_FILE_ATTRIBUTES;
    const bool have_title = GetFileAttributesA(k_title_font) != INVALID_FILE_ATTRIBUTES;
#else
    const bool have_body = false, have_title = false;
#endif
    if (have_body)
        g_font_body = io.Fonts->AddFontFromFileTTF(k_body_font, 17.0f);
    if (have_title)
        g_font_title = io.Fonts->AddFontFromFileTTF(k_title_font, 28.0f);
    if (!g_font_body || !g_font_title) {
        ImFont *def = io.Fonts->AddFontDefault();
        if (!g_font_body)  g_font_body  = def;
        if (!g_font_title) g_font_title = def;
    }

    pit::ui::ApplyStyle();

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    bool running = true;
    int frames = 0;
    std::vector<unsigned char> shot;
    int shot_w = 0, shot_h = 0;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_WINDOWEVENT &&
                       event.window.event == SDL_WINDOWEVENT_CLOSE &&
                       event.window.windowID == SDL_GetWindowID(window)) {
                running = false;
            }
        }

        if (app.worker_running && !app.busy.load()) {
            FinishRun(&app);
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        DrawUI(&app);

        ImGui::Render();

        int dw = 0, dh = 0;
        SDL_GetRendererOutputSize(renderer, &dw, &dh);
        SDL_SetRenderDrawColor(renderer, 5, 8, 15, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

        if (!screenshot.empty() && frame_limit > 0 && frames + 1 >= frame_limit) {
            unsigned char *argb = ReadPixels(renderer, dw, dh);
            if (argb) {
                shot.assign(argb, argb + (size_t)dw * dh * 4);
                shot_w = dw;
                shot_h = dh;
                std::free(argb);
            }
        }

        SDL_RenderPresent(renderer);

        frames++;
        if (frame_limit > 0 && frames >= frame_limit) {
            running = false;
        }
    }

    if (!screenshot.empty() && !shot.empty()) {
        if (WriteBMP(screenshot.c_str(), shot_w, shot_h, shot.data())) {
            std::printf("wrote %s (%dx%d)\n", screenshot.c_str(), shot_w, shot_h);
        }
    }

    if (app.worker_running) {
        app.worker.join();
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
