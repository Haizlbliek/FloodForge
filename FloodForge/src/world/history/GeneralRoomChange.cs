using FloodForge.World;

namespace FloodForge.History;

public class GeneralRoomChange<T> : MultipleRoomChange {
	protected readonly Func<Room, T> getter;
	protected readonly Action<Room, T> setter;
	protected readonly List<T> undoValues = [];
	protected readonly List<T> redoValues = [];

	public GeneralRoomChange(Func<Room, T> getter, Action<Room, T> setter) {
		this.getter = getter;
		this.setter = setter;
	}

	[Obsolete("Use AddRoom(Room, T) instead")]
	public new void AddRoom(Room room) {
		throw new NotImplementedException("Use AddRoom(Room, T) instead");
	}

	public virtual void AddRoom(Room room, T redoValue) {
		base.AddRoom(room);

		this.undoValues.Add(this.getter(room));
		this.redoValues.Add(redoValue);
	}

	public override void Undo() {
		for (int i = 0; i < this.rooms.Count; i++) {
			this.setter(this.rooms[i], this.undoValues[i]);
		}
	}

	public override void Redo() {
		for (int i = 0; i < this.rooms.Count; i++) {
			this.setter(this.rooms[i], this.redoValues[i]);
		}
	}
}