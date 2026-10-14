/*
 * Launcher settings persistence.
 *
 * Settings live in a tiny key=value file, so a user can read or hand-edit them
 * and a future version can add keys without breaking an old one. The file holds
 * only preferences - paths, the selected mod, toggles - never cartridge data.
 */

#ifndef PIT_UI_LAUNCHER_CONFIG_H
#define PIT_UI_LAUNCHER_CONFIG_H

#include <string>

namespace pit {
namespace ui {

struct Config {
    std::string source_rom;   /* last chosen source cartridge */
    std::string output_rom;   /* last chosen prepared copy */
    std::string mods_dir;     /* folder scanned for mod profiles; empty = mods beside the launcher */
    std::string mod_id;       /* "", or a discovered profile id such as "hard_mode" */
    bool  export_assets;      /* run the generated-data export after preparing */
    bool  vsync;              /* swap interval */
    bool  show_log;           /* show the log panel on the HOME view */
    float ui_scale;           /* 0.8 .. 1.6 */
};

void ConfigDefaults(Config &cfg);

/* The per-user settings path, creating its directory. Falls back to the working
 * directory when the environment gives no profile location. */
std::string ConfigPath();

/* Both return false on failure and leave cfg untouched on a failed load. A
 * missing file is not an error: the caller starts from defaults. */
bool ConfigLoad(Config &cfg, const std::string &path);
bool ConfigSave(const Config &cfg, const std::string &path);

} /* namespace ui */
} /* namespace pit */

#endif /* PIT_UI_LAUNCHER_CONFIG_H */
