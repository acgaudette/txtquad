./bootstrap is a script I wrote for convenience only.
It will automatically download, build, and install
the correct version/configuration of glfw
as well as the [default font](https://github.com/acgaudette/kufont-ascii).
However, there is nothing stopping you
from satisfying these dependencies manually.

If you would like to build using the provided harness,
you will need clang, glslc, and ninja.
Otherwise, the library is trivially architected
and building it by hand is not difficult.
Take a look at the [build.ninja](build.ninja) file for the invocations I use.

Note that in addition, glfw requires cmake to build.

libtxtquad builds with all three of the following platforms on CI.
Taking a look [there](.github/workflows/main.yml)
and comparing against the github runner docs
can be a good place to start if you're having issues.

# LINUX #

1. sh bootstrap
2. ninja so
3. ninja demos
4. bin/demos

or,

1. sh bootstrap
2. sh runex

# MACOS #

Building on macos is nearly identical to linux.
./bootstrap should "just work", but you may need:
- wget
- nproc

1. sh bootstrap
2. ninja dylib
3. ninja demos.macos
4. bin/demos.macos

# WINDOWS #

I ported ./bootstrap to powershell, again just as a convenience.
I can't guarantee it remains up to date with the linux version.
On windows, the bootstrap script will additionally
softlink your vulkan 1.2.x SDK installation into ./ext.

libtxtquad depends on runtime symbol resolution via weak linking,
and this is only partially supported outside of linux.
Later I will probably manually resolve symbols or hotload a main dll,
so this should be less of an issue.
For now, libtxtquad is built as a static library (.lib) on windows.

1. ./bootstrap.ps1
2. ninja lib
3. ninja demos.exe
4. bin/demos.exe

Remember to build in a clean shell,
as microsoft's default settings might clobber the build configuration.
