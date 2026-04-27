using FloodForge.World;

namespace FloodForge.History;

public class TagChange : MultipleRoomChange {
	protected List<HashSet<string>> undoValues = [];
	protected List<HashSet<string>> redoValues = [];

	[Obsolete("Use AddRoom(Room, HashSet<string>) instead")]
	public new void AddRoom(Room room) {
		throw new NotSupportedException("Use AddRoom(Room, HashSet<string>) instead");
	}

	public virtual void AddRoom(Room room, HashSet<string> redoValue) {
		base.AddRoom(room);
		this.undoValues.Add(room.data.tags);
		this.redoValues.Add(redoValue);
	}

	public override void Undo() {
		for (int i = 0; i < this.rooms.Count; i++) {
			this.rooms[i].data.tags = this.undoValues[i];
		}
	}

	public override void Redo() {
		for (int i = 0; i < this.rooms.Count; i++) {
			this.rooms[i].data.tags = this.redoValues[i];
		}
	}
}