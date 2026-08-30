using FloodForge.Popups;
using FloodForge.History;

namespace FloodForge.World;

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
			newRoom.DevPosition = this.relevantRoom.DevPosition + Vector2.NegY * 5;
			newRoom.CanonPosition = this.relevantRoom.CanonPosition + Vector2.NegY * 5;
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