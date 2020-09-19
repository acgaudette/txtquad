txtquad is a simple vulkan renderer built to do one thing:
render "game jam" style 3D text in the most straightforward way possible.

![](media/2bp.gif)

It supports a custom format that essentially boils down
to a square, ASCII-indexed binary bitmap.
The font file is encoded as a pbm, and WYSIWYG.

Doodling your own fonts is easy,
as long as you're in the market for monospace.
Or, you can draw sprites, and render those instead.
The font I made for the above gif
can be found [here](https://github.com/acgaudette/kufont-ascii).

txtquad is feature sparse but functional.
Maybe someday it won't create its own window.

# Getting Started

_The instructions below are for linux._
_For macos and windows, see [PLATFORMS.md](PLATFORMS.md)._

```
Dependencies:
    libglfw.so    3.3.2
    libvulkan.so  1.2.x
```

This project uses clang, git LFS, and the ninja buildsystem.
You will additionally require the vulkan headers + libs, and glslc.

1. ./bootstrap to download, build, and install glfw3 into ./ext
   - This also grabs the font from the link above
     and installs it into ./assets
     if a font doesn't already exist at that location
2. ./runex to build and launch ./bin/demos
   - This will build ./bin/libtxtquad.so as a side effect
   - Alternatively,
     running "ninja so" in the root dir
     will build the .so by itself

# Configuration

- Library settings can be found in ./config.h
- See the $config var in ./build.ninja
  to adjust compiler flags and switches
  (the lib is compiled in debug mode by default)
- Runtime configuration
  is possible via the Settings struct
  (see below)

# Usage

- Link against libtxtquad.so in your app
- Set up an assets directory
  containing the font and the compiled shaders,
  or just softlink the one from the repo (./assets)
  - The default shaders are compiled into here along with the demos

# API

`txtquad_init(struct Settings)`
- Call this from your main function to boot up the engine
- See the struct definition in ./txtquad.h
  for more details regarding configuration

`txtquad_start()`
- Call this to pass control
  from your app to the engine

`txtquad_update(struct Frame, struct Text*)`
- Implement this callback,
  it's called once per frame by the engine
- Grab your animation data from the Frame
- Write to the Text* to render stuff
  (it's just a pointer to a blob of static memory)

# Notes

- glfw is compiled statically into the binary by default
- libtxtquad resolves user callback symbols at runtime via weak linking.
  You _must_ implement txtquad_update(),
  otherwise the lib will panic.
- There is one example executable, but four demos.
  The demo selection can be controlled at compilation time by a define,
  or the $demo var in ./build.ninja
- This repo has some other orbiting code in it,
  like a corny single-header math library.
  Maybe I'll move those out later.

# Using the input module

- Include inp.h in your application's source
- When compiling txtquad,
  define INP_KEYS for keyboard + mouse press/hold/release polling macros,
  INP_TEXT for a text entry callback,
  and/or INP_MOUSE for a mouse position callback
  - Input support is compiled into the binary by default
    (see ./build.ninja) using these defines
- You will also need the glfw header as a dev dependency
- See ./inp.h and ./examples for API usage
  - You _must_ implement inp_ev_text()
    if libtxtquad is compiled with INP_TEXT,
    and/or inp_ev_mouse()
    if libtxtquad is compiled with INP_MOUSE,
    otherwise the lib will panic.
