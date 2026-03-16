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

### Instalação (macOS / Homebrew)

```bash
brew install cmake glfw freetype libvterm yaml-cpp pkg-config
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

**macOS:** Se o CMake não encontrar o yaml-cpp, use o prefix do Homebrew antes de `cmake ..`:

```bash
cmake -DCMAKE_PREFIX_PATH="$(brew --prefix)" ..
make
./ramterm
```

Para instalar (opcional):

```bash
sudo make install
```

## Configuração

O RamTerm procura o arquivo de configuração na seguinte ordem (primeiro encontrado):

| Sistema   | Caminho do config |
|----------|-------------------|
| Linux / Mac | `~/.config/ramterm/config.yaml` |
| Linux       | `/etc/ramterm/config.yaml` |
| Windows     | `%APPDATA%\ramterm\config.yaml` (ex.: `C:\Users\...\AppData\Roaming\ramterm\config.yaml`) |
| Desenvolvimento | `config/config.yaml` ou `../config/config.yaml` (relativo ao executável) |

Se nenhum for encontrado, usam-se os valores padrão.

Exemplo completo (tema Dracula, paleta ANSI, scrollback):

```yaml
window:
  width: 800
  height: 600
  # title: "RamTerm"

theme:
  use_default_theme: false
  background: { r: 40, g: 42, b: 54, a: 255 }
  font_color: { r: 248, g: 248, b: 242, a: 255 }
  selection: { r: 24, g: 248, b: 242, a: 89 }

  palette_0:  { r: 40,  g: 42,  b: 54  }
  palette_1:  { r: 255, g: 85,  b: 85  }
  palette_2:  { r: 80,  g: 250, b: 123 }
  palette_3:  { r: 241, g: 250, b: 140 }
  palette_4:  { r: 189, g: 147, b: 249 }
  palette_5:  { r: 255, g: 121, b: 198 }
  palette_6:  { r: 139, g: 233, b: 253 }
  palette_7:  { r: 248, g: 248, b: 242 }
  palette_8:  { r: 98,  g: 114, b: 164 }
  palette_9:  { r: 255, g: 85,  b: 85  }
  palette_10: { r: 80,  g: 250, b: 123 }
  palette_11: { r: 241, g: 250, b: 140 }
  palette_12: { r: 189, g: 147, b: 249 }
  palette_13: { r: 255, g: 121, b: 198 }
  palette_14: { r: 139, g: 233, b: 253 }
  palette_15: { r: 255, g: 255, b: 255 }

buffer_lines: 1000

font:
  path: "FiraCodeNerdFontMono-Regular.ttf"
  size: 16

shell: "bash"
```

- **window:** `width`, `height` em pixels; `title` opcional. O ícone da janela (dock/barra de tarefas) usa `assets/ramterm-logo.png` se existir (ao rodar do diretório do build, o programa procura em `../assets/`).
- **theme:** `use_default_theme: true` usa Tango Dark; `false` usa as cores acima. Cores em 0–255 (r, g, b; `a` só em `background`, `font_color` e `selection`). `palette_0`–`palette_15` = paleta ANSI (0–7 normais, 8–15 brilho alto).
- **theme.selection:** cor do destaque da seleção; alpha = transparência (ex.: 89 ≈ 0,35). Se omitido, usa `font_color` com alpha 89.
- **buffer_lines:** número de linhas de scrollback.
- **font.path:** nome do arquivo (ex.: `FiraCodeNerdFontMono-Regular.ttf`) ou caminho absoluto. Procura em `~/.local/share/fonts/`, `/usr/share/fonts/`, no macOS em `~/Library/Fonts/`, etc.
- **shell:** comando do shell (ex.: `bash`, `zsh`).

## Estrutura do projeto

```
RamTerm/
├── CMakeLists.txt
├── assets/
│   └── ramterm-logo.png # ícone da janela (dock/barra de tarefas)
├── config/
│   └── config.yaml      # configuração (tema, fonte, shell)
├── third_party/
│   └── stb_image.h      # carregador de imagem (PNG) para o ícone
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

RamTerm é distribuído sob a **Licença MIT**.

```
Copyright (c) 2025 RamTerm contributors

Permissão é concedida, gratuitamente, a qualquer pessoa que obtenha uma cópia
deste software e dos arquivos de documentação associados (o "Software"), para
tratar o Software sem restrições, incluindo, sem limitação, os direitos de
usar, copiar, modificar, fundir, publicar, distribuir, sublicenciar e/ou vender
cópias do Software, e permitir que as pessoas a quem o Software seja fornecido
o façam, sob as seguintes condições:

O aviso de copyright acima e este aviso de permissão devem ser incluídos em
todas as cópias ou partes substanciais do Software.

O SOFTWARE É FORNECIDO "COMO ESTÁ", SEM GARANTIA DE QUALQUER TIPO, EXPRESSA OU
IMPLÍCITA, INCLUINDO, MAS NÃO SE LIMITANDO ÀS GARANTIAS DE COMERCIALIZAÇÃO,
ADEQUAÇÃO A UM PROPÓSITO ESPECÍFICO E NÃO VIOLAÇÃO. EM NENHUM CASO OS AUTORES
OU DETENTORES DOS DIREITOS AUTORAIS SERÃO RESPONSÁVEIS POR QUALQUER RECLAMAÇÃO,
DANOS OU OUTRA RESPONSABILIDADE, SEJA EM AÇÃO DE CONTRATO, DELITO OU DE OUTRA
FORMA, DECORRENTE DE, OU EM CONEXÃO COM O SOFTWARE OU O USO OU OUTRAS NEGOCIAÇÕES
NO SOFTWARE.
```

Em inglês: [MIT License](https://opensource.org/licenses/MIT) — substitua "2025" e "RamTerm contributors" pelo ano e nome que desejar.
