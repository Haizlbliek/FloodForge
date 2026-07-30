using FloodForge.World;

namespace FloodForge.History;

// REVIEW - merge TimelineChange and TimelineTypeChange into one TimelineChange class
// that, or add a separate SimultaneousTimelineChange class that does everything currently done 
// within the room setting constructor lambdas for WorldWindow's Create Timeline Room (because oh stars that's a nightmare)
public class TimelineChange : MultipleRoomChange {
	protected bool add;
	protected string timeline;
	protected Connection? connection;
	protected DenLineage? lineage;
	protected ReplaceRoom? replaceRoom;

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

	public void AddReplaceRoom(ReplaceRoom replaceRoom) {
		this.replaceRoom = replaceRoom;
	}

	private void Insert() {
		if (this.connection != null) {
			this.connection.timeline.timelines.Add(this.timeline);
			this.connection.conditionalPopup?.InvokeOnTimelineChange(this.connection.timeline);
		}
		else if (this.lineage != null) {
			this.lineage.timeline.timelines.Add(this.timeline);
			this.lineage.conditionalPopup?.InvokeOnTimelineChange(this.lineage.timeline);
		}
		else if (this.replaceRoom != null) {
			this.replaceRoom.timeline.timelines.Add(this.timeline);
		}
		else {
			foreach (Room room in this.rooms) {
				room.timeline.timelines.Add(this.timeline);
				room.conditionalPopup?.InvokeOnTimelineChange(room.timeline);
			}
		}
	}

	private void Erase() {
		if (this.connection != null) {
			this.connection.timeline.timelines.Remove(this.timeline);
			this.connection.conditionalPopup?.InvokeOnTimelineChange(this.connection.timeline);
		}
		else if (this.lineage != null) {
			this.lineage.timeline.timelines.Remove(this.timeline);
			this.lineage.conditionalPopup?.InvokeOnTimelineChange(this.lineage.timeline);
		}
		else if (this.replaceRoom != null) {
			this.replaceRoom.timeline.timelines.Remove(this.timeline);
		}
		else {
			foreach (Room room in this.rooms) {
				room.timeline.timelines.Remove(this.timeline);
				room.conditionalPopup?.InvokeOnTimelineChange(room.timeline);
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
