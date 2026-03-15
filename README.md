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

O RamTerm procura o arquivo de configuração na seguinte ordem (primeiro encontrado):

| Sistema   | Caminho do config |
|----------|-------------------|
| Linux / Mac | `~/.config/ramterm/config.yaml` |
| Linux       | `/etc/ramterm/config.yaml` |
| Windows     | `%APPDATA%\ramterm\config.yaml` (ex.: `C:\Users\...\AppData\Roaming\ramterm\config.yaml`) |
| Desenvolvimento | `config/config.yaml` ou `../config/config.yaml` (relativo ao executável) |

Se nenhum for encontrado, usam-se os valores padrão.

Exemplo mínimo:

```yaml
window:
  width: 800
  height: 600

theme:
  use_default_theme: false   # true = Tango Dark
  background: { r: 40, g: 42, b: 54, a: 255 }
  font_color: { r: 248, g: 248, b: 242, a: 255 }
  # Cor do destaque da seleção (r, g, b, a 0–255; a = transparência). Se omitido, usa font_color com alpha 89.
  selection: { r: 248, g: 248, b: 242, a: 89 }

font:
  path: "FiraCodeNerdFontMono-Regular.ttf"   # só o nome ou caminho completo
  size: 16

shell: "bash"
```

- **font.path:** nome do arquivo (ex.: `FiraCodeNerdFontMono-Regular.ttf`) ou caminho absoluto. O programa procura em `~/.local/share/fonts/`, `/usr/share/fonts/`, etc.
- **theme:** `use_default_theme: true` usa Tango Dark; `false` usa as cores definidas (incluindo `palette_0`–`palette_15` para a paleta ANSI).
- **theme.selection:** cor do destaque da seleção de texto (r, g, b, a em 0–255). O alpha controla a transparência (ex.: 89 ≈ 0,35). Se omitido, usa a cor do texto com alpha 89.

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
