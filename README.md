# FloodForge

FloodForge is a C++ remake of a few Rain World modding tools.

It aims for intuitive controls, clean ui, and as few dependencies as possible.

## Building

### Windows

Requirements:

- [MSYS2 MINGW](https://www.msys2.org)

Open MSYS2 MINGW64 and run:
```
pcaman -Syu
pacman -S mingw-w64-x86_64-toolchain
pacman -S mingw-w64-x86_64-libutf8proc
```

#### One time build

If you want to have a permanent executable that you can run whenever, use this option.
`sh build.sh`

#### Building for debugging

Use this if you are editing the code and need to quickly test
`sh build.sh --debug`

### Shell script

The build.sh script also works under Msys2.

First, install Make:

```bash
pacman -S make
```

Then refer to the Linux build instructions.

### Linux

Install:

```bash
sudo apt-get install make
sudo apt-get install libglfw3-dev
sudo apt-get install pkg-config
sudo apt-get install g++
sudo apt-get install libutf8proc-dev
```

Build:

```bash
sh build.sh

# build in debug mode
sh build.sh --debug

# build in release mode
sh build.sh --release
```

## I found a bug!

Report it on the new [FloodForge Discord server](https://discord.gg/RBq8PDbCmB)!

## License

FloodForge is licensed under the [GPL-3.0 License](LICENSE).
Please refer to the `LICENSE` file for full details.

### GLFW License

GLFW binaries are included in this repository for ease-of-use.
The license is at the top of both `.h` files (`include/GLFW/glfw3.h`, `include/GLFW.glfw3native.h`).

### Asset Licenses

- Fonts: See associated `README` and license files in the `fonts/` directory.
- Bitmap Fonts: Generated using [Snow Bamboo](https://snowb.org).
- Splash Screen Art: Rendered from Rain World's Shoreline map.
- All other artwork: Hand-created by the FloodForge team.