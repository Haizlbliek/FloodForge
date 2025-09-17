# Tutorial

Welcome to FloodForge!
This is a tool developed by [Haizlbliek](https://github.com/haizlbliek) to help Rain World modders create and edit regions.
It aims for intuitive controls, clean ui, and as few dependencies as possible.

### 
## How to...

### Creating a new region
- `New`
- In the popup, type your region acronym
- `Confirm`

### 
### Importing an existing region
- `Import`
- Navigate to your `world\_xx.txt` file (`mods/YOUR_MOD/world/xx/world_xx.txt`)
- `Open`

### 
### Adding rooms to a region
*After creating or importing a region*
- `Add Room`
- Navigate to `mods/YOUR_MOD/world/xx-rooms/XX_A01.txt`
- `Open`

### 
### Connecting rooms
- Find two connections between two different rooms
- Right click and drag from the first connection to the second

### 
### Adding creatures to a den
- Choose a den, hover it, and press `C`
- In the popup, select the creature you wish to add
- Click multiple times for multiple of the same type of creature
- Press the right `+` to expand the `Tag` sidebar

### 
### Lineages & multiple types of creatures
*After opening a den*
- Press the left `+` to expand the `Lineages` sidebar
`<` -> Select previous lineage
`x` -> Delete selected lineage
`+` -> Add new lineage
`>` -> Select next lineage
- Creatures in the lineage are vertically placed
- Press the large `+` to add a new creature
- Press `...` to edit conditionals (See `Conditionals`)
- Press `x` to remove the creature from the lineage

### 
### Conditionals
- Either by pressing `D` while hovering a connection or room,
or by pressing `...` in a den lineage, the conditionals popup opens

**Connections and creatures:**
`ALL` -> This is visible to  *all*  slugcats
`ONLY` -> This is  *only*  visible to selected slugcats
`EXCEPT` -> This is visible to all slugcats  *except*  selected ones

**Rooms:**
`DEFAULT` -> This room has  *default*  visiblity
`EXCLUSIVE` -> This room is  *exclusive*  to selected slugcats
`HIDE` -> *Hide*  this room on selected slugcats


### 
### Creating a room
- Hover an empty area
- Press `R`
- Enter room name
- Select room size
- `Create`
- (See Droplet)

### 
### Droplet
Droplet is a Level Editor, made to speed up the region planning stage.
It supports most geometry, and a few dev tools objects.

# 
## Controls

### Any mode

Middle click + drag to move camera
Scroll to zoom

### 
### World Editor

X - Delete
C - Open creature den
G - Toggle visual merge
L - Change layer
T - Change tag
S - Change subregion
A - Change attractiveness
H - Toggle visibility
I - Move to back
D - Change conditionals
R - Open room in Droplet
Alt+T - Open Tutorial

### 
### Droplet

**Global**
1 - Switch to `Environment` tab
2 - Switch to `Geometry` tab
3 - Switch to `Cameras` tab

**Environment**
W - Set water level
LMB - Click and drag to move handles

**Geometry**
WASD - Move selected tool cursor
LMB - Draw
RMB - Erase
Shift+LMB - Rect draw
Shift+RMB - Rect erase
Q+LMB - Flood fill draw
Q+RMB - Flood fill erase

**Cameras**
LMB - Select camera
LMB+Drag - Move camera or camera angles
RMB - Reset camera angle
Shift - Hold to un-restrict camera angles
X - Delete selected camera
C - Add new camera


# 
# 
# 
# Knowledge Book

### 
## Room visual merge
When rooms are close to each other,
tiles may overlap and cover sections that should be visible.
Toggling  *merge*  on the overlapping rooms will draw their solid tiles behind everything else,
fixing the overlap.

### 
## Canon vs Dev positioning
Rain World has the ability to have two different position types:  *Canon*  and  *Dev.*
- **Canon**: Shown on the in-game map. Uses all three layers
so rooms in different layers can overlap.
- **Dev**: Used in tools like Cornifer and the dev map. Rooms are spread out to avoid overlap.

You can switch between modes with the `Canon`/`Dev` button.
Rooms are positioned according to the selected mode.
Hold ALT to display the room's other position, shown at half transparency.
Moving a room affects only the active mode. In order to move both positions, hold ALT while dragging.

### 
## Adding custom creatures
Some mods add creatures to the game, these will show up as `?` in FloodForge,
to show the proper icon and be able to add the creature to new rooms:

1. Add a folder inside `assets/creatures` with the mod name (e.g. `m4rblelous`).
2. Inside, put a .png image for every creature you want to add.
3. In `assets/creatures/mods.txt`, add a line with your directory name.

> **Side note:**
> Sometimes, mods add custom "parsings" for creature names, allowing alternate
> IDs to be used. An example of this are most Lizards; The Green Lizard can be put
> in the world file with either `GreenLizard` OR `Green`.
> 
> Adding your own is pretty simple,
> In `assets/creatures/parse.txt` add a line with the format:
> `Abbreviated Name>ActualID`
> 
> You can add as many as you like!

### 
## Adding custom slugcat timelines
To add a custom slugcat to FloodForge,
Upload your slugcat's icon into `assets/timelines` with the case-sensitive filename exactly equal
to the timeline ID.

E.g.  If you would use
`SillySlugcat : SU_A01 : SU_B01 : DISCONNECTED`
Then your timeline ID is `SillySlugcat`

### 
## Changing settings
Open `assets/settings.txt`, each line contains a setting key and value.
Settings are individually explained in comments above each key.