# FloodForge

Welcome to FloodForge!
This is a tool developed by [Haizlbliek](https://github.com/haizlbliek) to help Rain World modders edit regions.

## Instructions

### Creating a Region
- `New`
- Type region acronym
- `Confirm`

### Importing a Region
- `Import`
- Navigate to your `world\_xx.txt` file (`mods/YOUR_MOD/world/xx/world_xx.txt`)
- `Open`

### Adding Rooms to a Region
- `Add Room`
- Navigate to `mods/YOUR_MOD/world/xx-rooms/XX_A01.txt`
- `Open`

### Connecting Rooms
- Choose a connection (Magenta square)
- Right click and drag from the connection to another room's connection

## Controls

X - Delete
C - Creature Den
G - Toggle Visual Merge
L - Change Layer
T - Change Tag
S - Change Subregion
A - Change Attractiveness
H - Toggle Visibility
I - Move to back
ALT+T - Open Tutorial

## Canon vs Dev positioning
Rain World has the ability to have two different position types: *Canon* and *Dev*.
- **Canon**: Shown on the in-game map. Uses all three layers, so rooms in different layers can overlap.
- **Dev**: Used in tools like Cornifer and the dev map. Rooms are spread out to avoid overlap.

You can switch between modes with the `Canon`/`Dev` button.
Rooms are positioned according to the selected mode.
Hold ALT to display the room's other position, shown at half transparency.
Moving a room affects only the active mode. In order to move both positions, hold ALT while dragging.

> **Tip!**
> To hide this popup:
> - Go into `assets/settings.txt`
> - Add this line: `HideTutorial: true`