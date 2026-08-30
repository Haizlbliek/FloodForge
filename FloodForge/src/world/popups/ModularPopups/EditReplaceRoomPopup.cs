using FloodForge.Popups;
using FloodForge.History;
using System.Text.RegularExpressions;

namespace FloodForge.World;

public class EditReplaceRoomPopup : ModularPopup {
	private Room relevantRoom;
	private LabelContainer tutorialLabel;
	private ButtonContainer expandMenuButton;
	bool menuExpanded = false;

	private List<(ReplaceRoom replaceRoom, VerticalElement element)> associatedVerticalElements = [];

	private Timeline replaceRoomTimeline;
	private bool TimelineSelected => this.replaceRoomTimeline.timelines.Count != 0;

	private ButtonContainer timelineButton;
	private enum MenuMode {
		closed,
		createNew,
		fromRoom,
		fromFile
	}
	private MenuMode menuMode;
	private ButtonContainer createNewModeButton, fromRoomModeButton, fromFileModeButton;
	private HorizontalElement menuModeButtons;

	// createNew
	private string replaceRoomName;
	private Action<string>? updateNewNameSettingAction = _ => {};
	private StringSettingContainer replaceRoomNameSetting;
	private ButtonContainer generateReplaceRoomNameButton;
	private ButtonContainer createNewButton;

	// fromRoom
	private Room? selectedRoom;
	private ButtonContainer roomSelector;
	private LabelContainer selectedRoomLabel;
	private ButtonContainer createFromRoomButton;

	// fromFile
	private string filePath;
	private ButtonContainer openFilePopupButton;
	private LabelContainer filePathLabel;
	private ButtonContainer createFromFileButton;

	public EditReplaceRoomPopup(Room room) {
		this.relevantRoom = room;
		this.popupTitle = $"{room.name} - Edit Replacerooms";
		this.tutorialLabel = new LabelContainer("ReplaceRooms apply from top to bottom.\nThe last item in the list has priority.");
		this.expandMenuButton = new ButtonContainer("Add ReplaceRoom", () => { this.menuExpanded = !this.menuExpanded; if(this.menuExpanded) this.ResetReplaceRoomParameters(); }).SetContextCheck(b => { b.settingName = this.menuExpanded ? "Cancel" : "Add ReplaceRoom"; return true; });
		this.replaceRoomTimeline = new(TimelineType.Only, []);

		this.createNewModeButton = new ButtonContainer("create New", () => this.menuMode = MenuMode.createNew).SetContextCheck(_ => this.menuMode == MenuMode.createNew, true, false);
		this.fromRoomModeButton = new ButtonContainer("from Room", () => this.menuMode = MenuMode.fromRoom).SetContextCheck(_ => this.menuMode == MenuMode.fromRoom, true, false);
		this.fromFileModeButton = new ButtonContainer("from File", () => this.menuMode = MenuMode.fromFile).SetContextCheck(_ => this.menuMode == MenuMode.fromFile, true, false);
		this.menuModeButtons = new HorizontalElement([("cn", this.createNewModeButton), ("fr", this.fromRoomModeButton), ("ff", this.fromFileModeButton)], null, false, true);
		
		this.timelineButton = new ButtonContainer("Timeline", this.OpenTimelinePopup).SetContextCheck(b => {
			b.settingName = "Timeline" + (this.replaceRoomTimeline.timelines.Count == 0 ? "" : $" - {this.replaceRoomTimeline}"); this.RecalculateBounds(true, true); return true;});
		
		// createNew
		this.replaceRoomName = "";
		this.replaceRoomNameSetting = new StringSettingContainer("", name => this.replaceRoomName = name, ref this.updateNewNameSettingAction, prefix: $"{WorldWindow.region.acronym}_", hint: this.relevantRoom.name[(this.relevantRoom.name.IndexOf('_') + 1)..] + "Broken");
		this.generateReplaceRoomNameButton = new ButtonContainer("Generate", this.GenerateReplaceRoomName);
		this.createNewButton = new ButtonContainer("Create New", this.CreateNew).SetContextCheck(_ => {
			return this.replaceRoomName != "" && $"{WorldWindow.region.acronym}_" + this.replaceRoomName != this.relevantRoom.name && WorldWindow.region.rooms.FirstOrDefault(x => x.name == this.replaceRoomName) == null && WorldWindow.replaceReferenceRooms.FirstOrDefault(x => x.name == this.replaceRoomName) == null;
		});

		// fromRoom
		this.selectedRoom = null;
		this.roomSelector = new ButtonContainer("select Room to use", this.SelectRoomToUse).SetContextCheck(this.RoomSelectorCheck, true, true);
		this.selectedRoomLabel = new LabelContainer("No room selected");
		this.createFromRoomButton = new ButtonContainer("Create from Room", () => this.CreateReplaceRoom(this.selectedRoom!)).SetContextCheck(_ => this.selectedRoom != null && this.replaceRoomTimeline.timelines.Count != 0);

		// fromFile
		this.openFilePopupButton = new ButtonContainer("select File", this.OpenFileSystem);
		this.filePath = "";
		this.filePathLabel = new LabelContainer("None selected", autoCrop: true, fromRight: true);
		this.createFromFileButton = new ButtonContainer("Create from File", this.CreateFromFile).SetContextCheck(_ => File.Exists(this.filePath) && this.TimelineSelected, true);

		this.RebuildSettings(false);
	}

	public override void Draw() {
		Immediate.Color(Color.Yellow);
		UI.Line(new(this.bounds.x0, this.bounds.y1), (this.relevantRoom.Position - WorldWindow.cameraOffset) / WorldWindow.cameraScale);
		if (this.isHovered) // TODO - allow replaceRooms to reference these popups so that they can be rebuilt at any time
			this.RebuildSettings();
		base.Draw();
	}

	private void RebuildSettings(bool setPos = true) {
		this.settingContainers = [];
		this.associatedVerticalElements = [];
		this.AddToQueue(this.tutorialLabel);
		this.AddToQueue(new Divider());
		foreach (ReplaceRoom replacingRoom in this.relevantRoom.replaceRooms) {
			LabelContainer replaceRoomLabel = new LabelContainer("");
			TextureButtonContainer hideButton = new TextureButtonContainer("", UI.uiAtlas.UV(replacingRoom.setHidden ? "EyeClosed" : "EyeOpen"), () => this.ToggleReplaceRoomHidden(replacingRoom));
			ButtonContainer viewButton = new ButtonContainer("View", () => this.ViewReplaceRoom(replacingRoom));
			HorizontalElement reorderButtons = new ([("up", new ButtonContainer("/\\", () => this.Move(replacingRoom, true))), ("down", new ButtonContainer("\\/", () => this.Move(replacingRoom, false)))], null, false, true);
			HorizontalElement buttonElement = new ([("hide", hideButton), ("view", viewButton), ("movebuttons", reorderButtons)], [0.05f, 0f, 0.12f]);
			VerticalElement finalElement = new ([("label", replaceRoomLabel), ("buttons", buttonElement)]);
			this.associatedVerticalElements.Add((replacingRoom, finalElement));
			this.AddToQueue(finalElement);
		}
		this.AddToQueue(new Divider());
		this.AddToQueue(this.expandMenuButton);
		if (this.menuExpanded) {
			this.AddToQueue(this.timelineButton);
			if (this.TimelineSelected) {
				this.AddToQueue(this.menuModeButtons);
				this.AddToQueue(new Divider());
				switch (this.menuMode) {
					case MenuMode.createNew:
						this.AddToQueue(new LabelContainer("Not implemented yet."));
						this.AddToQueue(new HorizontalElement([("", this.replaceRoomNameSetting), ("", this.generateReplaceRoomNameButton)], [0f, UI.font.Measure("Generate", 0.03f).x + 0.01f]));
						this.AddToQueue(this.createNewButton);
					break;
					case MenuMode.fromRoom:
						this.AddToQueue(this.roomSelector);
						this.selectedRoomLabel.settingName = this.selectedRoom == null ? "No room selected" : $"Using {this.selectedRoom.name}";
						this.AddToQueue(this.selectedRoomLabel);
						this.AddToQueue(this.createFromRoomButton);
					break;
					case MenuMode.fromFile:
						this.AddToQueue(this.openFilePopupButton);
						this.filePathLabel.settingName = this.filePath == "" ? "None selected" : this.filePath;
						this.AddToQueue(this.filePathLabel);
						this.AddToQueue(this.createFromFileButton);
					break;
				}
			}
		}
		this.UpdateReplaceRoomLabels();
		this.AddQueuedSettings(setPos);
	}

	private void ToggleReplaceRoomHidden(ReplaceRoom replaceRoom) {
		replaceRoom.ToggleHide();
		this.RebuildSettings(true);
	}

	private void ResetReplaceRoomParameters() {
		this.replaceRoomTimeline = new(TimelineType.Only, []);
		this.replaceRoomName = "";
		this.selectedRoom = null;
		this.filePath = "";
		this.filePathLabel.settingName = "None selected";
		this.menuMode = MenuMode.closed;
	}

	private void OpenTimelinePopup() {
		PopupManager.Add(new TimelinePopup(this.replaceRoomTimeline, _ => {},
		(enabled, timeline) => {
			if (!enabled)
				this.replaceRoomTimeline.timelines.Add(timeline);
			else
				this.replaceRoomTimeline.timelines.Remove(timeline);
		}, true).SetButtons<TimelinePopup>("", "REPLACE", "").Translate(Mouse.Pos, false).Title("Specify ReplaceRoom Timeline"), true);
	}

	private void GenerateReplaceRoomName() {
		string generatedName = this.relevantRoom.name[(this.relevantRoom.name.IndexOf('_') + 1)..];
		generatedName += (this.replaceRoomTimeline.timelineType == TimelineType.Only ? "" : "X") + this.replaceRoomTimeline.timelines.FirstOrDefault();
		this.replaceRoomName = generatedName;
		this.updateNewNameSettingAction?.Invoke(generatedName);
		this.RecalculateBounds(true, true);
	}

	private void CreateNew() {
		string newPath = Path.Combine(WorldWindow.region.roomsPath, $"{WorldWindow.region.acronym}_{this.replaceRoomName}.txt");
		File.Copy(Path.Combine(WorldWindow.region.roomsPath, this.relevantRoom.name + ".txt"), newPath, true);

		WorldWindow.selectedDraggables = [];

		string key = "CreateReplaceRoomNew";
		WorldWindow.worldHistory.StartCollectingChanges([typeof(RoomAndConnectionChange)], key);
		Room addedRoom = WorldWindow.HandleRoomFilesSelected([newPath]).First();
		WorldWindow.worldHistory.StopCollectingChanges(key);

		addedRoom.isVirtualRoom = true;
		this.CreateReplaceRoom(addedRoom);
	}

	private bool RoomSelectorCheck(ButtonContainer button) {
		HashSet<WorldDraggable> selectedDraggables = WorldWindow.selectedDraggables;
		HashSet<WorldDraggable> relevantDraggables = [];
		foreach (WorldDraggable item in selectedDraggables) {
			if (item is Room and not OffscreenRoom or ReplaceRoom)
				relevantDraggables.Add(item);
		}
		if (relevantDraggables.Count != 1) {
			if (relevantDraggables.Count > 1)
				button.settingName = "Please select at most one room.";
			else
				button.settingName = this.selectedRoom != null ? "Room selected." : "Please select a room.";
			return false;
		}
		WorldDraggable draggableToSelect = relevantDraggables.First();
		if (draggableToSelect == this.relevantRoom) {
			button.settingName = "Please select a different room.";
			return false;
		}
		button.settingName = $"Click to select {(draggableToSelect is Room room ? room.name : (draggableToSelect as ReplaceRoom)?.replacingRoom.name ?? "NULL")}";
		return true;
	}

	private void SelectRoomToUse() {
		Room? roomToUse = (Room?)WorldWindow.selectedDraggables.FirstOrDefault(x => x is Room);
		roomToUse ??= (WorldWindow.selectedDraggables.FirstOrDefault(x => x is ReplaceRoom) as ReplaceRoom)?.replacingRoom;
		if (roomToUse == null)
			return;

		this.selectedRoom = roomToUse;
		this.selectedRoomLabel.settingName = this.selectedRoom == null ? "No room selected" : $"Using {this.selectedRoom.name}";
		this.RecalculateBounds(true, true);
		WorldWindow.selectedDraggables = [];
	}

	private void OpenFileSystem() {
		PopupManager.Add(new FilesystemPopup(this.SelectFile, 1).Hint("xx_a01_future.txt").Filter(new Regex("((?!.*_settings)(?=.+_.+).+\\.txt)|(gate_([^._-]+)_([^._-]+)\\.txt)")).ButtonText("Select").Title("Select room file to use"), true);
	}

	private void SelectFile(string[] selectedFileArray) {
		if (selectedFileArray.Length == 0)
			return;
		this.filePath = selectedFileArray.First();
		this.filePathLabel.settingName = this.filePath;
		this.RecalculateBounds(true, true);
	}

	// REVIEW - something goes wrong here
	// Now that I'm trying to find out why it's happening, I can't seem to recreate it. Glorious. Regardless, I'll describe the issue.
	// first, create a new replaceroom from file
	// second, notice that the replaceRoom does not seem to have actually appeared. (it is not rendering, at least.)
	// third, notice that opening the replaceRoom's settingspopup can still be summoned by clicking 'view' in the replaced room's popup
	// as I see now, there could be a few options:
	// the replacing room is not properly set/does not exist, thus the room doesn't draw. I'd expect this to cause different issues, however.
	// the replaceRoom is not added to WorldWindow.replaceRooms, thus it is not drawn but does exist
	private void CreateFromFile() {
		string newRoomName = Path.GetFileNameWithoutExtension(this.filePath);
		
		Room? existingRoom = null;
		foreach (Room room in WorldWindow.region.rooms) {
			if (room.name == newRoomName) {
				existingRoom = room;
				break;
			}
		}
		if (existingRoom != null) {
			PopupManager.Add(new InfoPopup($"A room with the name\n'{newRoomName}'\nalready exists\ncreate from room instead"));
			return;
		}
		
		string key = "CreateReplaceRoomFromFile";
		WorldWindow.worldHistory.StartCollectingChanges([typeof(RoomAndConnectionChange)], key);
		Room addedRoom = WorldWindow.HandleRoomFilesSelected([this.filePath]).First();
		WorldWindow.worldHistory.StopCollectingChanges(key);
		addedRoom.isVirtualRoom = true;
		this.CreateReplaceRoom(addedRoom);
	}

	private void CreateReplaceRoom(Room roomToUse) {
		ReplaceRoom newReplaceRoom = new(roomToUse, this.relevantRoom, this.replaceRoomTimeline, []) {
			DevPosition = this.relevantRoom.DevPosition + Vector2.NegY * 5,
			CanonPosition = this.relevantRoom.CanonPosition + Vector2.NegY * 5
		};
		ReplaceRoomChange replaceRoomChange = new(true);
		replaceRoomChange.AddReplaceRoom(newReplaceRoom);
		WorldWindow.worldHistory.Apply(replaceRoomChange);
		this.Close();
		this.CheckForWarnings(newReplaceRoom);
	}

	private void CheckForWarnings(ReplaceRoom newReplaceRoom) {
		string replacingRoom = newReplaceRoom.replacingRoom.name;
		string replacedRoom = newReplaceRoom.replacedRoom.name;
		List<string> warnings = [];
		if (newReplaceRoom.replacedRoom.roomExits.Count != newReplaceRoom.replacingRoom.roomExits.Count)
			warnings.Add($"New ReplaceRoom {replacingRoom}\nhas different exit count from {replacedRoom}");
		if (newReplaceRoom.replacedRoom.dens.Count != newReplaceRoom.replacingRoom.dens.Count)
			warnings.Add($"New ReplaceRoom {replacingRoom}\nhas different den count from {replacedRoom}");
		
		if (warnings.Count == 0)
			return;
		string finalWarningString = "";
		bool first = true;
		foreach (string item in warnings) {
			finalWarningString += (first ? "" : "\n---\n") + item;
			first = false;
		}
		PopupManager.Add(finalWarningString);
	}

	private void Move(ReplaceRoom replaceRoomToMove, bool up) {
		int direction = up ? -1 : 1;

		int originalRoomIndex = 0;
		int newRoomIndex = 0;
		ReplaceRoom? replaceRoomToSwapWith = null;

		for (; originalRoomIndex < this.relevantRoom.replaceRooms.Count; originalRoomIndex++) {
			if (this.relevantRoom.replaceRooms[originalRoomIndex] == replaceRoomToMove) {
				newRoomIndex = Math.Clamp(originalRoomIndex + direction, 0, this.relevantRoom.replaceRooms.Count - 1);
				replaceRoomToSwapWith = this.relevantRoom.replaceRooms[newRoomIndex];
				break;
			}
		}
		if (replaceRoomToSwapWith == null || replaceRoomToSwapWith == replaceRoomToMove)
			return;
		
		int originalWorldIndex = 0;
		int newWorldIndex = 0;
		for (int i = 0; i < WorldWindow.replaceRooms.Count; i++) {
			if (WorldWindow.replaceRooms[i] == replaceRoomToMove)
				originalWorldIndex = i;
			if (WorldWindow.replaceRooms[i] == replaceRoomToSwapWith)
				newWorldIndex = i;
		}
		
		// swap the relevant items
		WorldWindow.worldHistory.Apply(new MassChange([new ListSwapChange<ReplaceRoom>(this.relevantRoom.replaceRooms, originalRoomIndex, newRoomIndex), new ListSwapChange<ReplaceRoom>(WorldWindow.replaceRooms, originalWorldIndex, newWorldIndex)], () => this.RebuildSettings()));
	}

	private void ViewReplaceRoom(ReplaceRoom replaceRoomToView) {
		PopupManager.Add(new ReplaceRoomSettingsPopup(replaceRoomToView));
	}

	private void UpdateReplaceRoomLabels() {
		foreach ((ReplaceRoom replaceroom, VerticalElement verticalElement) in this.associatedVerticalElements) {
			string preProcessorConditionsToString = "";
			bool first = true;
			foreach (string condition in replaceroom.preProcessorConditions) {
				preProcessorConditionsToString += (first ? "" : ",") + condition;
			}
			preProcessorConditionsToString = preProcessorConditionsToString == "" ? "" :  $"{{{preProcessorConditionsToString}}}";
			verticalElement.GetByID("label")?.settingName = $"{preProcessorConditionsToString}({replaceroom.timeline}) -> {replaceroom.replacingRoom.name}";
		}
		this.RecalculateBounds(true, true);
	}
}