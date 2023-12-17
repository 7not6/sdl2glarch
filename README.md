# sdlgl2arch

sdl2glarch is a small libretro frontend based on the work of heuripedes nanoarch
https://github.com/heuripedes/nanoarch
It only provides the required (video,audio and basic input) features to run most
non-libretro-gl cores and there's an minimal UI with some core option configuration support.

you'll only need `sdl2` and `opengl` development files installed.

Below you will find the original content of heuripedes README.md file.

# nanoarch

nanoarch is a small libretro frontend (nanoarch.c has less than 1000 lines of
code) created for educational purposes. It only provides the required (video,
audio and basic input) features to run most non-libretro-gl cores and there's
no UI or configuration support.

## Building

Other than `make`, `pkg-config` and a working C99 or C++ compiler, you'll need
`alsa` and `glfw` development files installed.

## Running

    ./nanoarch <core> <uncompressed content>

