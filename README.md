# FloodForge

FloodForge is a project that is meant to remake the current Rain World tools in C++.

It aims for intuitive controls, clean ui, and as few dependencies as possible.


## Controls

| Action            | Key       | Description                                                   |
|-------------------|-----------|---------------------------------------------------------------|
| Delete            | `X`       | Removes hovered room or connection.                           |
| Change Room Layer | `L`       | Switches between layers within the current room.              |
| Print Debug Info  | `D`       | Outputs debug information for the hovered room or connection. |
| Cancel/Exit       | `ESC`     | Closes menus or cancels actions.                              |
| Accept            | `ENTER`   | Confirms selections or actions.                               |

## Building
### Windows
Requirements:
- [MSYS2 MINGW](https://www.msys2.org)

#### One time build
If you want to have a permanent executable that you can run whenever, use this option.
`build\Build.bat`

#### Building for debugging
Use this if you are editing the code and need to quickly test
`build\Build.bat --debug`

### Linux
> [!WARNING]  
> Linux builds are untested, they may not work.

Requirements:
- [GTK](https://www.gtk.org)

`./build/World.sh`

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