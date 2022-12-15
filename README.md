# WebGL "planetarium"

N circles fly around, all affecting each other by gravity and collision. All meaningful calculations are done on the GPU.

Full credit for inspiration and target goes to the old Java toy on [DAN-BALL](https://dan-ball.jp/en/javagame/planet/) by [ha55ii](http://hassii.blog39.fc2.com). This is far less full-featured!

## Building

Requirements:
- Linux: GCC and SDL2
- Windows: MinGW and Windows SDL2
- Web: Emscripten SDK (`emcc` and `file_packager` are both called), as well as a local webserver, e.g. Apache

Instructions:
- `make linux` and run `main`
- `make win` and run Main.exe
- `make web` and serve the output on a local webserver

`makefile` has some filepaths hard-coded for my build environment, so probably won't work without edits. See in particular `SYSROOT_WIN`, which should point to a directory containing C headers and SDL2 binaries and DLLs. If you don't have `emcc` and want to try it on a Linux device, comment out the definition of `EM_PACKAGER` or `make` will fail.

Tested on:
- Intel HD Graphics 530/Fedora 35/desktop
- Intel HD Graphics 530/Fedora 35/Chromium
- Intel UHD Graphics 620/Windows 10/desktop
- Intel UHD Graphics 620/Windows 10/Chromium
