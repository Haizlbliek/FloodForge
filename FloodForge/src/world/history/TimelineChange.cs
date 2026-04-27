using FloodForge.World;

namespace FloodForge.History;

public class TimelineChange : MultipleRoomChange {
	protected bool add;
	protected string timeline;
	protected Connection? connection;
	protected DenLineage? lineage;

	public TimelineChange(bool add, string timeline) {
		this.add = add;
		this.timeline = timeline;
	}

	public void AddConnection(Connection connection) {
		this.connection = connection;
	}

	public void AddLineage(DenLineage lineage) {
		this.lineage = lineage;
	}

	private void Insert() {
		if (this.connection != null) {
			this.connection.timelines.Add(this.timeline);
			this.connection.conditionalPopup?.InvokeOnTimelineChange(this.connection.timelineType, this.connection.timelines);
		}
		else if (this.lineage != null) {
			this.lineage.timelines.Add(this.timeline);
			this.lineage.conditionalPopup?.InvokeOnTimelineChange(this.lineage.timelineType, this.lineage.timelines);
		}
		else {
			foreach (Room room in this.rooms) {
				room.Timelines.Add(this.timeline);
				room.conditionalPopup?.InvokeOnTimelineChange(room.TimelineType, room.Timelines);
			}
		}
	}

	private void Erase() {
		if (this.connection != null) {
			this.connection.timelines.Remove(this.timeline);
			this.connection.conditionalPopup?.InvokeOnTimelineChange(this.connection.timelineType, this.connection.timelines);
		}
		else if (this.lineage != null) {
			this.lineage.timelines.Remove(this.timeline);
			this.lineage.conditionalPopup?.InvokeOnTimelineChange(this.lineage.timelineType, this.lineage.timelines);
		}
		else {
			foreach (Room room in this.rooms) {
				room.Timelines.Remove(this.timeline);
				room.conditionalPopup?.InvokeOnTimelineChange(room.TimelineType, room.Timelines);
			}
		}
	}

	public override void Undo() {
		if (this.add)
			this.Erase();
		else
			this.Insert();
	}

	public override void Redo() {
		if (this.add)
			this.Insert();
		else
			this.Erase();
	}
}
