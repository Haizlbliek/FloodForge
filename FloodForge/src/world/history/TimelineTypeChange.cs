namespace FloodForge.World;

public class TimelineTypeChange : MultipleRoomChange {
	protected Connection? connection;
	protected DenLineage? lineage;
	protected List<TimelineType> undoValues = [];
	protected TimelineType undoValue;
	protected TimelineType redoValue;

	public TimelineTypeChange(TimelineType redoValue) {
		this.redoValue = redoValue;
	}

	public override void AddRoom(Room room) {
		base.AddRoom(room);

		this.undoValues.Add(room.TimelineType);
	}

	public void AddConnection(Connection connection) {
		this.connection = connection;
		this.undoValue = this.connection.timelineType;
	}

	public void AddLineage(DenLineage lineage) {
		this.lineage = lineage;
		this.undoValue = this.lineage.timelineType;
	}

	public override void Undo() {
		this.connection?.timelineType = this.undoValue;
		this.connection?.conditionalPopup?.InvokeOnTimelineChange(this.connection.timelineType, this.connection.timelines);
		this.lineage?.timelineType = this.undoValue;
		this.lineage?.conditionalPopup?.InvokeOnTimelineChange(this.lineage.timelineType, this.lineage.timelines);
		for (int i = 0; i < this.rooms.Count; i++) {
			this.rooms[i].TimelineType = this.undoValues[i];
			this.rooms[i].conditionalPopup?.InvokeOnTimelineChange(this.rooms[i].TimelineType, this.rooms[i].Timelines);
		}
	}

	public override void Redo() {
		this.connection?.timelineType = this.redoValue;
		this.connection?.conditionalPopup?.InvokeOnTimelineChange(this.connection.timelineType, this.connection.timelines);
		this.lineage?.timelineType = this.redoValue;
		this.lineage?.conditionalPopup?.InvokeOnTimelineChange(this.lineage.timelineType, this.lineage.timelines);
		foreach (Room room in this.rooms) {
			room.TimelineType = this.redoValue;
			room.conditionalPopup?.InvokeOnTimelineChange(room.TimelineType, room.Timelines);
		}
	}
}