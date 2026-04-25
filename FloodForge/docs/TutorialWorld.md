# Tutorial

Welcome to FloodForge!
This is a tool developed by [Haizlbliek](https://github.com/haizlbliek) to help Rain World modders create and edit regions.
It aims for intuitive controls, clean ui, and as few dependencies as possible.


### 
## Controls

Middle click + drag to move camera
Scroll to zoom

Ctrl+Z - Undo
Ctrl+Y - Redo
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
F - Search for room
Alt+T - Open Tutorial
Alt+S - Open Splash
Right click - Open reference image settings; Reset popup size


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

**World View**
- Pressing the `Timeline` button in the top bar shows a similar menu.
This decides what conditionals are shown.
`ALL` -> Shows *all* conditionals
`ONLY` -> Shows *only* the conditionals visible to the selected slugcats.
`EXCEPT` -> Shows all conditionals *except* those
limited to `ONLY` the selected slugcats.
> **More clarification:**
> `ALL` will never hide any rooms.
> `ONLY` will *hide all rooms* if no slugcat is selected.
> `EXCEPT` will only hide conditionals set to `ONLY`.

### 
### Creating a room
- Hover an empty area
- Press `R`
- Enter room name
- Select room size
- `Create`
- (See Droplet)


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
Moving a room affects only the active mode.
In order to move both positions, hold ALT while dragging.

### 
## Default vs Path connections
Floodforge can visualise connections in two ways: *Default* and *Path.*
- **Default**: Same as the in-game map. Connections go from the
entrance of one shortcut to the entrance of the other.
- **Path**: Connections start at the end of a shortcut's path, 
usually a little distance from the shortcut's entrance.
In a way this better visualises the 'actual' geometry of a connection.

- This does not affect the in-game map.
It only serves to reduce visual clutter while in Floodforge.

###
## Adding Reference images
When making a region, you may have made a rough (or very polished) plan.
FloodForge allows you to use such an image as reference.
To add a new reference image:

1. `Add Reference`
2. Navigate to the relevant image
3. `Open`

The reference image will behave similarly to rooms, but is *not* exported
and is *not* preserved when loading another region or closing Floodforge.
To resize the image, right click it and drag the `Scale` slider.
To delete the image, similarly to a room, select or hover over it and press `X`.

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