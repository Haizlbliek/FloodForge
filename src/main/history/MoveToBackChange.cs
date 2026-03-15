namespace FloodForge.World;

public class MoveToBackChange : Change {
	protected Room room;
	protected int initialOffset;

	public MoveToBackChange(Room room) {
		this.room = room;
		this.initialOffset = WorldWindow.region.rooms.IndexOf(room);
	}

	public override void Undo() {
		WorldWindow.region.rooms.Remove(this.room);
		WorldWindow.region.rooms.Insert(this.initialOffset, this.room);
	}

	public override void Redo() {
		WorldWindow.region.rooms.Remove(this.room);
		WorldWindow.region.rooms.Add(this.room);
	}
}