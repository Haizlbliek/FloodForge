using FloodForge.Popups;
using FloodForge.History;
using FloodForge.Droplet;
using StbImageWriteSharp;
using static FloodForge.Main;

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
			this.AddToQueue(new HorizontalElement([this.newNameSetting, this.generateNewNameButton], [0f, UI.font.Measure("Generate", 0.03f).x + 0.01f]));

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