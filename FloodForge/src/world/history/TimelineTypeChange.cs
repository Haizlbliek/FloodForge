using FloodForge.World;

namespace FloodForge.History;

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

		this.undoValues.Add(room.timeline.timelineType);
	}

	public void AddConnection(Connection connection) {
		this.connection = connection;
		this.undoValue = this.connection.timeline.timelineType;
	}

	public void AddLineage(DenLineage lineage) {
		this.lineage = lineage;
		this.undoValue = this.lineage.timeline.timelineType;
	}

	public override void Undo() {
		this.connection?.timeline.timelineType = this.undoValue;
		this.connection?.conditionalPopup?.InvokeOnTimelineChange(this.connection.timeline);
		this.lineage?.timeline.timelineType = this.undoValue;
		this.lineage?.conditionalPopup?.InvokeOnTimelineChange(this.lineage.timeline);
		for (int i = 0; i < this.rooms.Count; i++) {
			this.rooms[i].timeline.timelineType = this.undoValues[i];
			this.rooms[i].conditionalPopup?.InvokeOnTimelineChange(this.rooms[i].timeline);
		}
	}

	public override void Redo() {
		this.connection?.timeline.timelineType = this.redoValue;
		this.connection?.conditionalPopup?.InvokeOnTimelineChange(this.connection.timeline);
		this.lineage?.timeline.timelineType = this.redoValue;
		this.lineage?.conditionalPopup?.InvokeOnTimelineChange(this.lineage.timeline);
		foreach (Room room in this.rooms) {
			room.timeline.timelineType = this.redoValue;
			room.conditionalPopup?.InvokeOnTimelineChange(room.timeline);
		}
	}
}