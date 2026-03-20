# Custom Themes

**How to make:**

In `assets/themes/` (the folder this file is in), add a folder with your theme name.
An example could be `assets/themes/test/`

In your theme folder, there are four files.

1. `theme.txt`
	The default file that all themes **must** include.
	It contains the colors for pretty much everything.

2. `geometry.png`
	The tile textures for most geometry types.
	Mostly used in Droplet.

3. `ui.png`
	Every special ui image is in here.

4. `uiIcons.png`
	Some very special icons that are rarely used.
	Usually, this doesn't need to be messed with.


**`theme.txt`**

Background             | Window background
Grid                   | Background grid
Border                 | All ui element borders (Usually for popups and buttons)
BorderHighlight        | `Border` but when focused/hovered
Popup                  | Popup background
PopupHeader            | Popup header
Button                 | Button background
ButtonDisabled         | Button background when disabled
Text                   | Text
TextDisabled           | Text when disabled (Usually in a button)
TextHighlight          | Text when focused/hovered
SelectionBorder        | Selection area border & room border when selected
RoomBorder             | Room border when not selected
RoomBorderHighlight    | Room border when hovered
RoomAir                | Air in rooms
RoomSolid              | Solid in rooms
RoomLayer2Solid        | Solid layer 2 in rooms
RoomPole               | Poles in rooms
RoomPlatform           | Platforms in rooms
RoomShortcutEnterance  | Shortcut entrances in rooms
RoomShortcutDot        | Shortcut dots in rooms
RoomShortcutRoom       | Room exits in rooms
RoomShortcutDen        | Dens in rooms
RoomConnection         | Connections
RoomConnectionHover    | Connections when hovered
Layer0Color            | Special color multiplied into rooms when on layer 0
Layer1Color            | Special color multiplied into rooms when on layer 1
Layer2Color            | Special color multiplied into rooms when on layer 2