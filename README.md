# FloodForge

FloodForge is a C++ remake of a few Rain World modding tools.

It aims for intuitive controls, clean ui, and as few dependencies as possible.

## Controls

| Action            | Key       | Description                                                       |
|-------------------|-----------|-------------------------------------------------------------------|
| Move Room / Popup | `LMB`     | Move rooms around, hold ALT to move without snapping.             |
| Move Camera       | `MMB`     | Pan camera.                                                       |
| Connect Rooms     | `RMB`     | Add connections between room exits.                               |
| Delete            | `X`       | Removes hovered room or connection.                               |
| Creature Den      | `C`       | Opens the hovered den.                                            |
| Room Merge        | `G`       | Toggle room visual merging.                                       |
| Change Room Layer | `L`       | Switches between layers within the hovered room.                  |
| Change Flag       | `F`       | Change room flags (shelter, karma gate, scavenger outpost, etc.). |
| Change Subregion  | `S`       | Openes a popup for adding, removing, and changing subregions.     |
| Hide / Show       | `H`       | Toggle visibility of hovered room.                                |
| Show other rooms  | `I`       | Places hovered room behind all other rooms.                       |
| Cancel/Exit       | `ESC`     | Closes menus or cancels actions.                                  |
| Accept            | `ENTER`   | Confirms selections or actions.                                   |

## Building

### Windows

Requirements:

- [MSYS2 MINGW](https://www.msys2.org)

#### One time build

If you want to have a permanent executable that you can run whenever, use this option.
`./Build.bat`

#### Building for debugging

Use this if you are editing the code and need to quickly test
`./Build.bat --debug`

### Shell script

The build.sh script also works under Msys2.

First, install Make:

```bash
pacman -S make
```

Then refer to the Linux build instructions.

### Linux
>
> [!WARNING]  
> Linux builds are untested, they may not work.

Requirements:

- GLFW3 (`sudo apt install libglfw3-dev`)
- pkg-config
- Make

```bash
./Build.sh

# build in debug mode
./Build.sh --debug

# build in release mode
./Build.sh --release
```

## Settings

Settings are stored in `assets/settings.txt`
To switch to a different theme, change the `Theme:` value to the name of a directory in `assets/themes`

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
