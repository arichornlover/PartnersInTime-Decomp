#include "ui/launcher_config.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <direct.h>
#endif

namespace pit {
namespace ui {

void ConfigDefaults(Config &cfg)
{
    cfg.source_rom.clear();
    cfg.output_rom.clear();
    cfg.mods_dir = "mods";
    cfg.mod_id.clear();
    cfg.export_assets = true;
    cfg.vsync = true;
    cfg.show_log = true;
    cfg.ui_scale = 1.0f;
}

static bool EnsureDir(const std::string &dir)
{
#ifdef _WIN32
    if (_mkdir(dir.c_str()) == 0) {
        return true;
    }
    /* Already there is fine; anything else is reported by the caller. */
    return errno == EEXIST;
#else
    (void)dir;
    return true;
#endif
}

std::string ConfigPath()
{
    const char *appdata = std::getenv("APPDATA");
    if (appdata && appdata[0] != '\0') {
        std::string dir = std::string(appdata) + "\\PiT-Launcher";
        EnsureDir(dir);
        return dir + "\\config.ini";
    }
    return "pit-launcher.ini";
}

static void Trim(std::string &s)
{
    size_t b = s.find_first_not_of(" \t\r\n");
    size_t e = s.find_last_not_of(" \t\r\n");
    if (b == std::string::npos) {
        s.clear();
    } else {
        s = s.substr(b, e - b + 1);
    }
}

bool ConfigLoad(Config &cfg, const std::string &path)
{
    std::ifstream in(path);
    if (!in) {
        return false;
    }

    Config c;
    ConfigDefaults(c);

    std::string line;
    while (std::getline(in, line)) {
        size_t hash = line.find('#');
        if (hash != std::string::npos) {
            line = line.substr(0, hash);
        }
        Trim(line);
        if (line.empty()) {
            continue;
        }
        size_t eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        Trim(key);
        Trim(val);

        if (key == "source_rom")      c.source_rom = val;
        else if (key == "output_rom") c.output_rom = val;
        else if (key == "mods_dir")   c.mods_dir = val;
        else if (key == "mod_id")     c.mod_id = val;
        else if (key == "export_assets") c.export_assets = (val == "1" || val == "true");
        else if (key == "vsync")      c.vsync = (val == "1" || val == "true");
        else if (key == "show_log")   c.show_log = (val == "1" || val == "true");
        else if (key == "ui_scale")   c.ui_scale = std::strtof(val.c_str(), nullptr);
    }

    if (c.ui_scale < 0.7f || c.ui_scale > 1.8f) {
        c.ui_scale = 1.0f;
    }
    if (c.mods_dir.empty()) {
        c.mods_dir = "mods";
    }
    cfg = c;
    return true;
}

bool ConfigSave(const Config &cfg, const std::string &path)
{
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        return false;
    }
    out << "# Partners in Time launcher settings\n";
    out << "source_rom=" << cfg.source_rom << "\n";
    out << "output_rom=" << cfg.output_rom << "\n";
    out << "mods_dir=" << cfg.mods_dir << "\n";
    out << "mod_id=" << cfg.mod_id << "\n";
    out << "export_assets=" << (cfg.export_assets ? 1 : 0) << "\n";
    out << "vsync=" << (cfg.vsync ? 1 : 0) << "\n";
    out << "show_log=" << (cfg.show_log ? 1 : 0) << "\n";
    out << "ui_scale=" << cfg.ui_scale << "\n";
    return out.good();
}

} /* namespace ui */
} /* namespace pit */
