#include "Config.hpp"

#include <filesystem>
#include <yaml-cpp/yaml.h>

#include <exception>
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>

namespace {
    std::string getHomeDir() {
#if defined(_WIN32)
        const char* p = std::getenv("USERPROFILE");
        return p ? std::string(p) : "";
#else
        const char* p = std::getenv("HOME");
        return p ? std::string(p) : "";
#endif
    }

    std::string expandTilde(std::string path) {
        if (path.empty() || path[0] != '~') return path;
        std::string home = getHomeDir();
        if (home.empty()) return path;
        if (path.size() == 1 || path[1] == '/' || path[1] == '\\') {
            path.replace(0, 1, home);
            return path;
        }
        return path;
    }

    bool isOnlyFilename(const std::string& s) {
        return s.find('/') == std::string::npos && s.find('\\') == std::string::npos;
    }

    /** Diretórios base onde procurar fontes (nome do arquivo), por SO. */
    std::vector<std::string> getFontSearchBases() {
        std::vector<std::string> bases;
        std::string home = getHomeDir();
#if defined(_WIN32)
        const char* localAppData = std::getenv("LOCALAPPDATA");
        if (localAppData) {
            bases.push_back(std::string(localAppData) + "\\Microsoft\\Windows\\Fonts\\");
        }
        bases.push_back("C:\\Windows\\Fonts\\");
        if (!home.empty()) {
            bases.push_back(home + "\\AppData\\Local\\Microsoft\\Windows\\Fonts\\");
        }
#elif defined(__APPLE__)
        bases.push_back("/Library/Fonts/");
        bases.push_back("/System/Library/Fonts/");
        if (!home.empty()) {
            bases.push_back(home + "/Library/Fonts/");
        }
        bases.push_back("/Network/Library/Fonts/");
#else
        bases.push_back("/usr/share/fonts/");
        bases.push_back("/usr/local/share/fonts/");
        if (!home.empty()) {
            bases.push_back(home + "/.local/share/fonts/");
            bases.push_back(home + "/.fonts/");
        }
        bases.push_back("/usr/share/fonts/truetype/");
        bases.push_back("/usr/share/fonts/opentype/");
        if (!home.empty()) {
            bases.push_back(home + "/.local/share/fonts/truetype/");
            bases.push_back(home + "/.local/share/fonts/opentype/");
        }
#endif
        return bases;
    }

    /** Subpastas comuns onde fontes podem estar dentro de cada base. */
    std::vector<std::string> getFontSubdirs() {
#if defined(_WIN32)
        return { "" };
#else
        return { "", "truetype/", "TTF/", "opentype/", "OTF/", "dejavu/", "dejavu-sans-fonts/", "liberation/" };
#endif
    }

    /** Procura o arquivo de fonte nos diretórios padrão do SO; retorna caminho completo ou vazio. */
    std::string resolveFontFile(const std::string& filename) {
        if (filename.empty() || !isOnlyFilename(filename)) return filename;
        std::vector<std::string> bases = getFontSearchBases();
        std::vector<std::string> subdirs = getFontSubdirs();
        for (const std::string& base : bases) {
            for (const std::string& sub : subdirs) {
                std::string full = base + sub;
                if (!sub.empty() && full.back() != '/' && full.back() != '\\') full += "/";
                full += filename;
                if (std::filesystem::exists(full)) return full;
            }
        }
        return "";
    }

    /** Nome do arquivo de fonte padrão por SO (só nome + extensão). */
    std::string getDefaultFontFilename() {
#if defined(_WIN32)
        return "consola.ttf";
#elif defined(__APPLE__)
        return "Menlo.ttc";
#else
        return "DejaVuSansMono.ttf";
#endif
    }

    /** Fallbacks completos (caminhos) por SO quando o nome não for encontrado. */
    std::vector<std::string> getDefaultFontFallbacks() {
        std::vector<std::string> out;
#if defined(_WIN32)
        out = { "C:\\Windows\\Fonts\\consola.ttf", "C:\\Windows\\Fonts\\lucon.ttf", "C:\\Windows\\Fonts\\arial.ttf" };
#elif defined(__APPLE__)
        out = { "/Library/Fonts/Menlo.ttc", "/System/Library/Fonts/Monaco.ttf", "/Library/Fonts/Courier New.ttf" };
#else
        std::string home = getHomeDir();
        out = {
            "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
            "/usr/share/fonts/dejavu/DejaVuSansMono.ttf",
            "/usr/share/fonts/liberation/LiberationMono-Regular.ttf",
        };
        if (!home.empty()) {
            out.push_back(home + "/.local/share/fonts/DejaVuSansMono.ttf");
            out.push_back(home + "/.fonts/DejaVuSansMono.ttf");
        }
#endif
        return out;
    }

    float parseFloat(const YAML::Node& node) {
        if (!node || !node.IsDefined()) return 0.0f;
        if (node.Type() != YAML::NodeType::Scalar) return 0.0f;

        try {
            return static_cast<float>(node.as<double>());
        } catch (...) {}

        try {
            return static_cast<float>(node.as<int>());
        } catch (...) {}
        
        return 0.0f;
    }
}

AppConfig Config::load(const std::string& path) {
    AppConfig config;
    const std::string fullPath = resolveConfigPath(path);
    if (fullPath.empty()) {
        return config;
    }

    try {
        const YAML::Node root = YAML::LoadFile(fullPath);

        if (const auto window = root["window"]) {
            if (window["width"]) {
                int w = window["width"].as<int>();
                if (w >= 80 && w <= 7680) config.window.width = w;
            }
            if (window["height"]) {
                int h = window["height"].as<int>();
                if (h >= 24 && h <= 4320) config.window.height = h;
            }
            if (window["title"]) {
                config.window.title = "RamTerm - " + window["title"].as<std::string>();
            }
        }

        if (const auto theme = root["theme"]) {
            if (theme["use_default_theme"]) {
                try {
                    config.theme.use_default_theme = theme["use_default_theme"].as<bool>();
                } catch (...) {}
            }

            if (const auto background = theme["background"]) {
                if (background["r"]) config.theme.background.r = parseFloat(background["r"]);
                if (background["g"]) config.theme.background.g = parseFloat(background["g"]);
                if (background["b"]) config.theme.background.b = parseFloat(background["b"]);
                if (background["a"]) config.theme.background.a = parseFloat(background["a"]);
                if (config.theme.background.a <= 0.f) config.theme.background.a = 255.f;
                const float sum = config.theme.background.r + config.theme.background.g + config.theme.background.b;
                if (sum < 13.f) {
                    config.theme.background.r = 13.f;
                    config.theme.background.g = 13.f;
                    config.theme.background.b = 15.f;
                }
            }

            if (const auto font = theme["font_color"]) {
                if (font["r"]) config.theme.font.r = parseFloat(font["r"]);
                if (font["g"]) config.theme.font.g = parseFloat(font["g"]);
                if (font["b"]) config.theme.font.b = parseFloat(font["b"]);
                if (font["a"]) config.theme.font.a = parseFloat(font["a"]);
                if (config.theme.font.a <= 0.f) config.theme.font.a = 255.f;
                const float sum = config.theme.font.r + config.theme.font.g + config.theme.font.b;
                if (sum < 13.f) {
                    config.theme.font.r = 255.f;
                    config.theme.font.g = 255.f;
                    config.theme.font.b = 255.f;
                }
            }

            bool anyPaletteKey = false;
            for (int i = 0; i < 16; ++i) {
                std::string key = "palette_" + std::to_string(i);
                if (const auto pal = theme[key]) {
                    anyPaletteKey = true;
                    if (pal["r"]) config.theme.palette[i].r = parseFloat(pal["r"]);
                    if (pal["g"]) config.theme.palette[i].g = parseFloat(pal["g"]);
                    if (pal["b"]) config.theme.palette[i].b = parseFloat(pal["b"]);
                }
            }
            if (!config.theme.use_default_theme && !anyPaletteKey) {
                ThemeConfig tango = getTangoDarkTheme();
                config.theme.palette = tango.palette;
            }
        } else {
            config.theme = getTangoDarkTheme();
            config.theme.use_default_theme = true;
        }

        if (const auto font = root["font"]) {
            if (font["path"]) {
                std::string path = font["path"].as<std::string>();
                if (!path.empty()) {
                    path = expandTilde(path);
                    if (isOnlyFilename(path)) {
                        std::string resolved = resolveFontFile(path);
                        if (!resolved.empty()) {
                            config.font.path = std::move(resolved);
                        } else {
                            std::string defaultResolved = resolveFontFile(getDefaultFontFilename());
                            if (!defaultResolved.empty()) {
                                config.font.path = std::move(defaultResolved);
                                std::cerr << "[RamTerm] Fonte nao encontrada: " << path << " — usando padrao do SO.\n";
                            } else {
                                config.font.path = std::move(path);
                            }
                        }
                    } else {
                        config.font.path = std::move(path);
                    }
                }
            }
            if (font["size"]) {
                int size = font["size"].as<int>();
                if (size >= 6 && size <= 72) {
                    config.font.size = size;
                }
            }
        }

        if (const auto shellNode = root["shell"]) {
            std::string shell = shellNode.as<std::string>();
            if (!shell.empty()) {
                config.shell = std::move(shell);
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "Aviso: falha ao carregar '" << path << "': " << error.what() << '\n';
        std::cerr << "Usando configuracao padrao.\n";
        return AppConfig{};
    }

    if (!fileExists(config.font.path)) {
        std::string tried = config.font.path;
        bool usedFallback = false;
        for (const std::string& fb : getDefaultFontFallbacks()) {
            if (fileExists(fb)) {
                config.font.path = fb;
                usedFallback = true;
                break;
            }
        }
        if (usedFallback) {
            std::cerr << "[RamTerm] Fonte nao encontrada: " << tried << " — usando fallback do SO.\n";
        } else if (isOnlyFilename(tried)) {
            std::cerr << "[RamTerm] Fonte nao encontrada: " << tried << " (procure em font.path no config.yaml: so nome+extensao ou caminho completo).\n";
        }
    }

    return config;
}

AppConfig Config::loadFromSearchPaths(const std::vector<std::string>& basePaths) {
    for (const auto& base : basePaths) {
        std::string fullPath = resolveConfigPath(base);
        if (!fullPath.empty()) {
            return load(base);
        }
    }
    if (std::getenv("PWD") != nullptr) {
        std::string pwdPath = std::string(std::getenv("PWD")) + "/config/config";
        if (!resolveConfigPath(pwdPath).empty()) return load(pwdPath);
    }
    AppConfig def = AppConfig{};
    def.theme = getTangoDarkTheme();
    def.theme.use_default_theme = true;
    std::string defaultResolved = resolveFontFile(getDefaultFontFilename());
    if (!defaultResolved.empty()) {
        def.font.path = std::move(defaultResolved);
    }
    if (!fileExists(def.font.path)) {
        for (const std::string& fb : getDefaultFontFallbacks()) {
            if (fileExists(fb)) {
                def.font.path = fb;
                break;
            }
        }
    }
    return def;
}

bool Config::fileExists(const std::string& path) {
    return std::filesystem::exists(path);
}

std::string Config::resolveConfigPath(const std::string& basePath) {
    const std::string yamlPath = basePath + ".yaml";
    const std::string ymlPath = basePath + ".yml";

    if (fileExists(yamlPath)) {
        return yamlPath;
    }

    if (fileExists(ymlPath)) {
        return ymlPath;
    }

    return "";
}

ThemeConfig Config::getTangoDarkTheme() {
    ThemeConfig t;
    t.use_default_theme = true;
    t.background.r = 46.f;
    t.background.g = 52.f;
    t.background.b = 54.f;
    t.background.a = 255.f;
    t.font.r = 211.f;
    t.font.g = 215.f;
    t.font.b = 207.f;
    t.font.a = 255.f;
    // Paleta Tango Dark (0–7 normal, 8–15 bright)
    const float tango[16][3] = {
        { 0.f, 0.f, 0.f },           // 0 black
        { 204.f, 0.f, 0.f },        // 1 red
        { 78.f, 154.f, 6.f },       // 2 green
        { 196.f, 160.f, 0.f },      // 3 yellow
        { 52.f, 101.f, 164.f },     // 4 blue
        { 117.f, 80.f, 123.f },     // 5 magenta
        { 6.f, 152.f, 154.f },     // 6 cyan
        { 211.f, 215.f, 207.f },   // 7 white
        { 85.f, 87.f, 83.f },      // 8 bright black
        { 239.f, 41.f, 41.f },     // 9 bright red
        { 138.f, 226.f, 52.f },    // 10 bright green
        { 252.f, 233.f, 79.f },    // 11 bright yellow
        { 114.f, 159.f, 207.f },   // 12 bright blue
        { 173.f, 127.f, 168.f },   // 13 bright magenta
        { 52.f, 226.f, 226.f },    // 14 bright cyan
        { 238.f, 238.f, 236.f },   // 15 bright white
    };
    for (int i = 0; i < 16; ++i) {
        t.palette[i].r = tango[i][0];
        t.palette[i].g = tango[i][1];
        t.palette[i].b = tango[i][2];
    }
    return t;
}
