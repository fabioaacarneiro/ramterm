# RamTerm

Emulador de terminal em C++ para Linux (e outros sistemas com suporte a GLFW/OpenGL). Usa **libvterm** para interpretação do protocolo e **FreeType** para renderização de fontes.

## Funcionalidades

- Tema configurável (Dracula, Tango Dark ou paleta personalizada)
- Entrada UTF-8 (acentos, japonês, CJK etc.)
- Scrollback com rolagem pelo mouse e barra de rolagem
- Configuração via YAML (fonte, cores, shell, janela)
- Suporte a caracteres de bloco (U+2580–U+259F) e Nerd Fonts

## Dependências

- CMake ≥ 3.20
- C++20
- OpenGL
- [GLFW](https://www.glfw.org/) 3
- [FreeType](https://freetype.org/) 2
- [libvterm](https://github.com/neovim/libvterm)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp)

### Instalação (Fedora)

```bash
sudo dnf install cmake gcc-c++ glfw-devel freetype-devel vterm-devel yaml-cpp-devel
```

### Instalação (Ubuntu/Debian)

```bash
sudo apt install cmake g++ libglfw3-dev libfreetype6-dev libvterm-dev libyaml-cpp-dev
```

## Build

```bash
git clone https://github.com/SEU_USUARIO/RamTerm.git
cd RamTerm
mkdir build && cd build
cmake ..
make
./ramterm
```

Para instalar (opcional):

```bash
sudo make install
```

## Configuração

O RamTerm procura `config/config.yaml` no diretório do executável ou em `../config/config`. Se não encontrar, usa valores padrão.

Exemplo mínimo:

```yaml
window:
  width: 800
  height: 600

theme:
  use_default_theme: false   # true = Tango Dark
  background: { r: 40, g: 42, b: 54, a: 255 }
  font_color: { r: 248, g: 248, b: 242, a: 255 }

font:
  path: "FiraCodeNerdFontMono-Regular.ttf"   # só o nome ou caminho completo
  size: 16

shell: "bash"
```

- **font.path:** nome do arquivo (ex.: `FiraCodeNerdFontMono-Regular.ttf`) ou caminho absoluto. O programa procura em `~/.local/share/fonts/`, `/usr/share/fonts/`, etc.
- **theme:** `use_default_theme: true` usa Tango Dark; `false` usa as cores definidas (incluindo `palette_0`–`palette_15` para a paleta ANSI).

## Estrutura do projeto

```
RamTerm/
├── CMakeLists.txt
├── config/
│   └── config.yaml      # configuração (tema, fonte, shell)
├── src/
│   ├── app/             # loop principal, PTY
│   ├── config/          # leitura do YAML
│   ├── font/            # FreeType, cache de glifos
│   ├── pty/             # spawn do shell
│   ├── renderer/        # OpenGL, células, scrollbar
│   ├── terminal/        # libvterm, scrollback
│   └── window/          # GLFW, input UTF-8
└── README.md
```

## Licença

[Definir licença — por exemplo MIT, GPL, etc.]
