using FloodForge.Popups;
using FloodForge.History;
using FloodForge.Droplet;
using StbImageWriteSharp;
using static FloodForge.Main;
using System.Text.RegularExpressions;

namespace FloodForge.World;

public class RoomSettingsPopup : ModularPopup {
	private Room relevantRoom;
	private BoolSettingContainer enclosedRoomToggle;
	private IntSliderSettingContainer waterLevelSlider;
	private BoolSettingContainer waterInFrontToggle;
	private ButtonContainer renderRoomButton;
	private ButtonContainer renameRoomButton;
	private ButtonContainer createTimelineRoomButton;
	private CreateTimelineRoomPopup? createTimelineRoomPopup;

	private ButtonContainer editReplaceRooms;
	private EditReplaceRoomPopup? editReplaceRoomPopup;

	public RoomSettingsPopup(Room relevantRoom) {
		this.relevantRoom = relevantRoom;

		this.enclosedRoomToggle = new BoolSettingContainer("Enclosed Room", this.relevantRoom.data.enclosedRoom, this.UpdateEnclosedRoom);
		this.AddToQueue(this.enclosedRoomToggle);
		this.waterLevelSlider = new IntSliderSettingContainer("Water Height", this.relevantRoom.data.waterHeight, -2, this.relevantRoom.height, this.UpdateWaterHeight).UpdateWhileDragging(true);
		this.AddToQueue(this.waterLevelSlider);
		this.waterInFrontToggle = new BoolSettingContainer("Water In Front", this.relevantRoom.data.waterInFront, b => {
			WorldWindow.worldHistory.Apply(new VariableChange<bool>(this.relevantRoom.data.waterInFront, b, bRedo => this.relevantRoom.data.waterInFront = bRedo));
		});
		this.AddToQueue(this.waterInFrontToggle);
		this.renderRoomButton = new ButtonContainer("Render Room", this.RenderRoom);
		this.AddToQueue(this.renderRoomButton);
		this.renameRoomButton = new ButtonContainer("Rename Room", this.RenameRoom);
		this.AddToQueue(this.renameRoomButton);
		this.createTimelineRoomButton = new ButtonContainer("Create Timeline Room", this.AddCreateTimelineRoomPopup);
		this.AddToQueue(this.createTimelineRoomButton);
		this.editReplaceRooms = new ButtonContainer("Edit ReplaceRooms", this.EditReplaceRooms);
		this.AddToQueue(this.editReplaceRooms);
		this.AddQueuedSettings();
	}

	private void UpdateEnclosedRoom(bool enclosed) {
		WorldWindow.worldHistory.Apply(new VariableChange<bool>(this.relevantRoom.data.enclosedRoom, enclosed, enclosedRedo => 
			this.relevantRoom.data.enclosedRoom = enclosedRedo
		));
	}

	private void UpdateWaterHeight(int newHeight) {
		WorldWindow.worldHistory.Apply(new VariableChange<int>(this.relevantRoom.data.waterHeight, newHeight, hRedo => {
			this.relevantRoom.data.waterHeight = hRedo;
			this.relevantRoom.RegenerateWater();
		}));
	}

	private void RenderRoom() {
		PopupManager.Add(new ConfirmPopup($"Render Room {this.relevantRoom}?\nThis will overwrite existing images.")).SetOkay("Render").Okay(() => {
			DropletWindow.LoadRoom(this.relevantRoom, Vector2.Zero);
			if (DropletWindow.Render(out string errorMessage, out (string name, string path, byte[] image)[] images)) {
				foreach ((string name, string path, byte[] image) in images) {
					FloodForge.Backup.File(path);

					using Stream stream = File.OpenWrite(path);
					ImageWriter writer = new();
					writer.WritePng(image, CameraTextureWidth, CameraTextureHeight, ColorComponents.RedGreenBlue, stream);
				}
				PopupManager.Add(new InfoPopup($"Render complete.\nBackups made."));
			}
			else {
				PopupManager.Add(new InfoPopup($"Error while rendering {this.relevantRoom.name}\n{errorMessage}\nview log.txt for more info"));
			}
		});
	}

	private void RenameRoom() {
		if (this.relevantRoom.data.tags.Contains("GATE") || this.relevantRoom.name.StartsWith("GATE")) {
			PopupManager.Add(new InfoPopup("Cannot rename GATE rooms!"));
		}
		else {
			PopupManager.Add(new RenameRoomPopup(this.relevantRoom, name => {
				if (NameChanger.ChangeRoomName(this.relevantRoom, name)) {
					PopupManager.Add(new InfoPopup($"Room successfully renamed to\n{name}"));
				}
				else {
					PopupManager.Add(new InfoPopup($"Problem encountered."));
				}
			}).Translate(Mouse.Pos, true).Title("Rename Room"));
		}
	}

	private void EditReplaceRooms() {
		this.editReplaceRoomPopup = (EditReplaceRoomPopup)new EditReplaceRoomPopup(this.relevantRoom).Translate(Mouse.Pos, false);
		PopupManager.Add(this.editReplaceRoomPopup);
	}

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
			
			this.openFilePopupButton = new ButtonContainer("Select File", this.OpenFileSystem);
			this.filePath = "";
			this.filePathLabel = new LabelContainer("None selected", autoCrop: true, fromRight: true);
			this.createFromFileButton = new ButtonContainer("Create from File", this.CreateFromFile).SetContextCheck(_ => File.Exists(this.filePath) && this.TimelineSelected, true);

			this.RebuildSettings(false);
		}

		public override void Draw() {
			Immediate.Color(Color.Yellow);
			UI.Line(new(this.bounds.x0, this.bounds.y1), (this.relevantRoom.Position - WorldWindow.cameraOffset) / WorldWindow.cameraScale);
			if (this.isHovered)
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
				ButtonContainer viewButton = new ButtonContainer("View", () => this.ViewReplaceRoom(replacingRoom));
				HorizontalElement reorderButtons = new ([("up", new ButtonContainer("/\\", () => this.Move(replacingRoom, true))), ("down", new ButtonContainer("\\/", () => this.Move(replacingRoom, false)))], null, false, true);
				HorizontalElement buttonElement = new ([("view", viewButton), ("movebuttons", reorderButtons)], [0f, 0.12f]);
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
						case MenuMode.fromRoom:
							this.AddToQueue(new LabelContainer("Not implemented yet."));
							this.AddToQueue(new LabelContainer("Not implemented yet."));
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

		private void ResetReplaceRoomParameters() {
			this.replaceRoomTimeline = new(TimelineType.Only, []);
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

		private void OpenFileSystem() {
			PopupManager.Add(new FilesystemPopup(this.SelectFile, 1).Hint("xx_a01_future.txt").Filter(new Regex("((?!.*_settings)(?=.+_.+).+\\.txt)|(gate_([^._-]+)_([^._-]+)\\.txt)")).Title("Select room file to use"), true);
		}

		private void SelectFile(string[] selectedFileArray) {
			if (selectedFileArray.Length == 0)
				return;
			this.filePath = selectedFileArray.First();
			this.filePathLabel.settingName = UI.font.CropText(this.filePath, this.usableBounds.x1 - this.usableBounds.x0, 0.03f, out _, true);
			this.RecalculateBounds(true, true);
		}

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
			ReplaceRoom newReplaceRoom = new(addedRoom, this.relevantRoom, this.replaceRoomTimeline, []);
			ReplaceRoomChange replaceRoomChange = new(true);
			replaceRoomChange.AddReplaceRoom(newReplaceRoom);
			WorldWindow.worldHistory.Apply(replaceRoomChange);
			this.Close();

			if (newReplaceRoom.replacedRoom.roomExits.Count != newReplaceRoom.replacingRoom.roomExits.Count)
				PopupManager.Add($"New ReplaceRoom {newReplaceRoom.replacingRoom.name}\nhas different exit count from {newReplaceRoom.replacedRoom.name}\n(this may cause issues)");
		}

		private void Move(ReplaceRoom replaceRoomToMove, bool up) {
			int direction = up ? -1 : 1;

			int originalRoomIndex = 0;
			int newRoomIndex = 0;
			ReplaceRoom? replaceRoomToSwapWith = null;

			for (; originalRoomIndex < this.relevantRoom.replaceRooms.Count; originalRoomIndex++) {
				if (this.relevantRoom.replaceRooms[originalRoomIndex] == replaceRoomToMove) {
					newRoomIndex = Math.Clamp(newRoomIndex + direction, 0, this.relevantRoom.replaceRooms.Count - 1);
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

	private void AddCreateTimelineRoomPopup() {
		this.createTimelineRoomPopup = (CreateTimelineRoomPopup)new CreateTimelineRoomPopup(this).SetSize(new(0.7f, 0f)).Translate(Mouse.Pos, false).Title("Create Timeline Room");
		PopupManager.Add(this.createTimelineRoomPopup);
	}

	public class CreateTimelineRoomPopup : ModularPopup {
		private Room relevantRoom;
		private RoomSettingsPopup parent;
		private bool copyConnections;
		private string newName;
		private Timeline newTimeline;
		private Action<string>? updateNewNameSettingAction = s => {};
		
		private ButtonContainer timelineButton;

		private StringSettingContainer newNameSetting;
		private ButtonContainer generateNewNameButton;

		private ButtonContainer createRoomButton;

		public CreateTimelineRoomPopup(RoomSettingsPopup parent) {
			this.relevantRoom = parent.relevantRoom;
			this.parent = parent;
			this.copyConnections = true;
			this.newName = "";
			this.newTimeline = new(TimelineType.Only, []);

			this.AddToQueue(new BoolSettingContainer("Copy Connections", this.copyConnections, b => this.copyConnections = b));

			this.timelineButton = new ButtonContainer("Timeline", this.OpenTimelinePopup).SetContextCheck(b => {
				b.settingName = "Timeline" + (this.newTimeline.timelines.Count == 0 ? "" : $" - {this.newTimeline}");
				this.RecalculateBounds(true, true);
				return true;
			});
			this.AddToQueue(this.timelineButton);

			this.newNameSetting = new StringSettingContainer("", name => this.newName = name, ref this.updateNewNameSettingAction, prefix: $"{WorldWindow.region.acronym}_", hint: this.relevantRoom.name[(this.relevantRoom.name.IndexOf('_') + 1)..]);
			this.generateNewNameButton = new ButtonContainer("Generate", this.GenerateNewName);
			this.AddToQueue(new HorizontalElement([("", this.newNameSetting), ("", this.generateNewNameButton)], [0f, UI.font.Measure("Generate", 0.03f).x + 0.01f]));

			this.createRoomButton = new ButtonContainer("Create Room", this.CreateRoom).SetContextCheck(_ => {
				return this.newName != "" && $"{WorldWindow.region.acronym}_" + this.newName != this.relevantRoom.name;
			});
			this.AddToQueue(this.createRoomButton);
			this.AddQueuedSettings();
		}
		
		private void OpenTimelinePopup() {
			PopupManager.Add(new TimelinePopup(this.newTimeline, type => this.newTimeline.timelineType = type == TimelineType.All ? TimelineType.Only : type,
			(enabled, timeline) => {
				if (!enabled)
					this.newTimeline.timelines.Add(timeline);
				else
					this.newTimeline.timelines.Remove(timeline);
			}, true).SetButtons<TimelinePopup>("", "EXCLUSIVE", "HIDE").Translate(Mouse.Pos, false).Title("Specify Timeline"));
		}

		private void GenerateNewName() {
			string generatedName = this.relevantRoom.name[(this.relevantRoom.name.IndexOf('_') + 1)..];
			generatedName += (this.newTimeline.timelineType == TimelineType.Only ? "" : "X") + this.newTimeline.timelines.FirstOrDefault();
			this.newName = generatedName;
			this.updateNewNameSettingAction?.Invoke(generatedName);
			this.RecalculateBounds(true, true);
		}

		private void CreateRoom() {
			string newPath = Path.Combine(WorldWindow.region.roomsPath, $"{WorldWindow.region.acronym}_{this.newName}.txt");
			File.Copy(Path.Combine(WorldWindow.region.roomsPath, this.relevantRoom.name + ".txt"), newPath, true);

			WorldWindow.selectedDraggables = [];

			string key = "TIMELINEROOM";
			WorldWindow.worldHistory.StartCollectingChanges([], key);

			Room[] newRooms = WorldWindow.HandleRoomFilesSelected([newPath]);
			Change[] foundChanges = WorldWindow.worldHistory.StopCollectingChanges(key);

			RoomAndConnectionChange addChange = new(adding: true);
			RoomAndConnectionChange removeChange = new(adding: false);
			List<Change> unmanagedChanges = [];
			foreach (Change foundChange in foundChanges) {
				if (foundChange is RoomAndConnectionChange roomChange) {
					foreach (Room room in roomChange.GetRooms()) {
						addChange.AddRoom(room);
					}
					foreach (Connection connection in roomChange.GetExternalConnections()) {
						addChange.AddConnection(connection);
					}
				}
				else {
					unmanagedChanges.Add(foundChange);
				}
			}

			WorldWindow.worldHistory.StartCollectingChanges([], key);
			WorldWindow.worldHistory.Apply(new MassChange([..unmanagedChanges])); // start and immediately apply the unmanaged changes so they'll get applied with the rest once collection finishes.
			if (newRooms.Length != 0) {
				Room newRoom = newRooms.First();
				newRoom.DevPosition = this.relevantRoom.DevPosition + Vector2.One;
				newRoom.CanonPosition = this.relevantRoom.CanonPosition + Vector2.One;
				newRoom.timeline = new(this.newTimeline);

				List<Change> tlModifications = [];

				Timeline newInverted = this.newTimeline.Inverted();
				Timeline newInvertedAndRoom = this.relevantRoom.timeline.And(newInverted);
				TimelineTypeChange roomTLTypeChange = new(newInvertedAndRoom.timelineType);
				roomTLTypeChange.AddRoom(this.relevantRoom);
				tlModifications.Add(roomTLTypeChange);

				foreach (string timeline in this.relevantRoom.timeline.timelines) {
					if (!newInvertedAndRoom.timelines.Contains(timeline)) {
						TimelineChange roomTLChange = new(false, timeline);
						roomTLChange.AddRoom(this.relevantRoom);
						tlModifications.Add(roomTLChange);
					}
				}
				foreach (string timeline in newInvertedAndRoom.timelines) {
					if (!this.relevantRoom.timeline.timelines.Contains(timeline)) {
						TimelineChange roomTLChange = new(true, timeline);
						roomTLChange.AddRoom(this.relevantRoom);
						tlModifications.Add(roomTLChange);
					}
				}
				
				if (this.copyConnections) {
					foreach (Connection connection in this.relevantRoom.connections) {
						if (!connection.timeline.OverlapsWith(this.newTimeline)) {
							continue;
						}

						Connection copiedConnection = new Connection(connection.roomA == this.relevantRoom ? newRoom : connection.roomA, connection.roomB == this.relevantRoom ? newRoom : connection.roomB, connection.roomAExitID, connection.roomBExitID) {
							timeline = this.newTimeline.And(connection.timeline)
						};
						addChange.AddConnection(copiedConnection);
						if (!connection.timeline.OverlapsWith(newInvertedAndRoom)) {
							removeChange.AddConnection(connection);
						}
						else {
							if (connection.timeline.timelineType == TimelineType.Only) {
								foreach (string timeline in this.newTimeline.timelines) {
									if (connection.timeline.timelines.Contains(timeline)) {
										TimelineChange connectionTimelineChange = new(false, timeline);
										connectionTimelineChange.AddConnection(connection);
										tlModifications.Add(connectionTimelineChange);
									}
								}
							}
							else {
								if (connection.timeline.timelineType == TimelineType.All) {
									TimelineTypeChange connectionTimelineTypeChange = new(TimelineType.Except);
									connectionTimelineTypeChange.AddConnection(connection);
									tlModifications.Add(connectionTimelineTypeChange);
								}
								foreach (string timeline in this.newTimeline.timelines) {
									if (!connection.timeline.timelines.Contains(timeline)) {
										TimelineChange connectionTimelineChange = new(true, timeline);
										connectionTimelineChange.AddConnection(connection);
										tlModifications.Add(connectionTimelineChange);
									}
								}
							}
						}
					}
				}
				WorldWindow.worldHistory.Apply(new MassChange([..tlModifications]));
			}
			WorldWindow.worldHistory.Apply(removeChange);
			WorldWindow.worldHistory.Apply(addChange);
			WorldWindow.worldHistory.GetAndApplyCollectedMassChange(key);
			this.parent.Close();
			this.Close();
		}
	}
}