# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Final project for INF01047 (Computação Gráfica e Visualização I) at UFRGS, by Anisio Lorentz and Antonio de Pádua. Target application is described in `SPEC.md`: a first-person 3D puzzle room where the user arranges geometric pieces (blocks, cylinders, ramp) to build a path to an elevated platform, referenced from the Superliminal video at `https://www.youtube.com/watch?v=HEBEQhwG-rU` (2:35–3:05). `COMPILACAO.md` is the upstream build doc from the course template.

## Working rules

- **Never create git commits in this repository.** The user will commit by hand. This applies even when changes look complete or the user says "done" — confirm explicitly before running any `git commit`, `git push`, or equivalent.
- **Document generated code thoroughly.** This is a coursework project; readability and explainability matter more than terseness. For any non-trivial code you write or modify, add clear comments in Portuguese (matching the existing style in `src/main.cpp`) explaining *what* the block does and *why*, especially around: shader uniform setup, matrix transformations, OpenGL state changes, collision math, camera logic, and Bézier evaluation. Prefer over-commenting to under-commenting here — the default "no comments unless non-obvious" rule does **not** apply to this repo.

## Build & Run

The repository is the standard FCG course template. Three build systems coexist (CMake, Linux `Makefile`, `Makefile.macOS`); pick the one matching the host. The executable is always named `main` and **must be run from its output directory** because asset paths in `src/main.cpp` are hardcoded as `../../data/...` (e.g. `bin/macOS/`, `bin/Linux/`, `bin/Debug/`).

- **Linux / CMake (preferred on Linux):** `cmake --workflow --preset configure-build-run` (or `cmake -B build -S . && cmake --build build && cmake --build build -- run`). Incremental rebuilds work.
- **macOS:** `make -f Makefile.macOS && make -f Makefile.macOS run`. ⚠️ Two known issues on Apple Silicon:
  1. The Makefile passes `-L/opt/homebrew/Cellar` (parent dir) instead of `-L/opt/homebrew/opt/glfw/lib`, so `-lglfw` fails to link.
  2. It omits `src/correcao.cpp`, so linking fails with `Correcao_KeyCallback` undefined.

  Workaround until the Makefile is fixed — invoke g++ directly:
  ```zsh
  g++ -std=c++11 -Wall -Wno-deprecated-declarations -Wno-unused-function -g \
      -I ./include/ -o ./bin/macOS/main \
      src/main.cpp src/glad.c src/textrendering.cpp src/tiny_obj_loader.cpp \
      src/stb_image.cpp src/correcao.cpp \
      -framework OpenGL -L/opt/homebrew/opt/glfw/lib \
      -lglfw -lm -ldl -lpthread
  ```
  GLFW must be installed first: `brew install glfw`.
- **Windows:** see `COMPILACAO.md`. Prebuilt GLFW static libs live in `lib-mingw-32/`, `lib-mingw-64/`, `lib-ucrt-64/`, `lib-vc2022/`; `CMakeLists.txt` auto-selects based on detected toolchain.

There are no tests, linters, or formatters — `-Wall` warnings from the compiler are the only signal. The huge wall of `-Wmissing-braces` warnings comes from `include/dejavufont.h` and is benign.

## Architecture

This is a single-binary OpenGL 3.3 application. The "engine" lives in **`src/main.cpp` (~1600 lines)** — everything (window setup, input, scene state, render loop, matrix stack helpers) is in that one translation unit. Treat it as the entry point for almost every change.

Key collaborators around `main.cpp`:
- **`src/correcao.cpp`** — single function `Correcao_KeyCallback` used by the professor's automated grading (Shift+0..9 exits with code 100..109). Must be called first inside `KeyCallback`. **Do not modify.**
- **`src/textrendering.cpp` + `include/dejavufont.h`** — bitmap font HUD overlay.
- **`src/glad.c`, `src/stb_image.cpp`, `src/tiny_obj_loader.cpp`** — vendored library impls (header-only libs whose `.cpp` is just an `#define IMPL` shim).
- **`src/buildtriangles.cpp`** — present in the tree but **not in any build's source list** (course template leftover). Do not add it without confirming intent.
- **`src/shader_vertex.glsl` / `src/shader_fragment.glsl`** — GLSL shaders loaded at runtime from `../../src/` relative to the executable. Editing them does not require a rebuild; restart is enough.
- **`include/`** — `matrices.h` and `utils.h` are the course's own math/util helpers; `glm/`, `glad/`, `GLFW/`, `KHR/`, `stb_image.h`, `tiny_obj_loader.h` are vendored third-party headers.
- **`data/`** — `.obj` meshes (`bunny`, `sphere`, `plane`) and `.jpg` textures. `FONTES.txt` documents attribution; keep it up to date when adding assets.

### Render-loop conventions inside `main.cpp`

The pipeline is the canonical course pattern; understanding it is enough to add any feature:

1. **Asset loading at startup** (`main()` around line 295+):
   - `LoadShadersFromFiles()` compiles the two GLSL files into one program.
   - `LoadTextureImage("../../data/...")` appends to a numbered texture array (`TextureImage0`, `TextureImage1`, ...).
   - For each `.obj`: `ObjModel m(path); ComputeNormals(&m); BuildTrianglesAndAddToVirtualScene(&m);`
2. **`g_VirtualScene`** — `std::map<std::string, SceneObject>` is the central registry. Each `BuildTrianglesAndAddToVirtualScene` call adds one entry per shape (keyed by shape name in the OBJ, e.g. `"the_sphere"`).
3. **Per-frame draw**: set the model matrix uniform, set an integer "object id" uniform that the fragment shader switches on for per-object shading, then `DrawVirtualObject("the_sphere")`.

When the SPEC features (puzzle pieces, ramp, platform, Bézier-animated decoration, look-at camera) are implemented, expect them all to land as additions to `main.cpp`, new OBJ/texture files in `data/`, and new branches inside the fragment shader's object-id switch. Splitting `main.cpp` would diverge from the course template — keep new logic localized within it unless asked otherwise.

### Coordinate conventions

The code uses GLM with right-handed coords and column-major matrices via `include/matrices.h`. Camera math currently in `main.cpp` is free-look first-person; the SPEC requires also adding a look-at camera mode toggle.
