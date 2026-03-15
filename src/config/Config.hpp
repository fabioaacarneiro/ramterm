#pragma once

#include <array>
#include <string>
#include <vector>

struct WindowConfig {
    int width = 1280;
    int height = 720;
    std::string title = "RamTerm";
};

/** Cor RGB em 0–255 (config.yaml). */
struct RGBColor {
    float r = 0.f;
    float g = 0.f;
    float b = 0.f;
};

/** Cores em 0–255 (como no config.yaml). */
struct BackgroundColor {
    float r = 13.f;
    float g = 13.f;
    float b = 15.f;
    float a = 255.f;
};

/** Cores em 0–255 (como no config.yaml). */
struct FontColor {
    float r = 255.f;
    float g = 255.f;
    float b = 255.f;
    float a = 255.f;
};

/** Paleta ANSI 0–15: 0=preto, 1=vermelho, 2=verde, 3=amarelo, 4=azul, 5=magenta, 6=cyan, 7=branco; 8–15=brilho alto. */
using ThemePalette = std::array<RGBColor, 16>;

struct ThemeConfig {
    /** true = usa tema padrão do terminal (Tango Dark); false = usa cores abaixo. */
    bool use_default_theme = false;
    BackgroundColor background;
    FontColor font;
    /** Cor do destaque da seleção (0–255; a = alpha, ex.: 89 para ~0.35). */
    FontColor selection;
    ThemePalette palette{};
};

struct FontSettings {
    std::string path = "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf";
    int size = 14;
};

struct AppConfig {
    ThemeConfig theme;
    WindowConfig window;
    FontSettings font;
    std::string shell = "bash";
};

class Config {
public:
    static AppConfig load(const std::string& path);
    /** Tenta carregar de cada caminho em ordem; usa padrões se nenhum existir. */
    static AppConfig loadFromSearchPaths(const std::vector<std::string>& basePaths);
    /** Caminhos de config por SO: Linux/Mac ~/.config/ramterm/config, Windows %APPDATA%\\ramterm\\config. */
    static std::vector<std::string> getDefaultConfigSearchPaths();
    /** Tema padrão do terminal (Tango Dark). Usado quando theme.use_default_theme == true. */
    static ThemeConfig getTangoDarkTheme();

private:
    static bool fileExists(const std::string& path);
    static std::string resolveConfigPath(const std::string& basePath);
};