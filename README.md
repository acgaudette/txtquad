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

# Building

```
Dependencies:
    libglfw.so    3.3.2
```

This project uses clang and the ninja buildsystem.

1. ./mkdep to download, build, and install glfw3 into ./ext
2. ./runex to build and launch ./bin/demos
   - This will build ./bin/libtxtquad.so as a side effect
   - Alternatively,
     running "ninja" in the root dir
     will build the .so by itself

# Configuration

- All settings can be found in ./config.h
- See the $config var in ./build.ninja
  to adjust compiler flags and switches
  (the lib is compiled in debug mode by default)

# Usage

- Link against libtxtquad.so in your app
- Set up an assets directory
  containing the font and the compiled shaders,
  or just softlink the one from the repo (./assets)
  - The default shaders are compiled into here along with the demos
  - If ./mkdep doesn't find a font in ./assets,
    it downloads the one from the link above

# API

`txtquad_init(<asset-path>)`
- Call this from your main function to boot up the engine,
  passing in the dir you'd like to load your spv and pbm files from

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
- You will also need the glfw header as a dev dependency
- See ./inp.h and ./examples for API usage
