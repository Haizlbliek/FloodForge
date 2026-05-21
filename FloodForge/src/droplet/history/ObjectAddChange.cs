using FloodForge.World;

namespace FloodForge.History;

public class ObjectAddChange : Change {
	readonly bool adding;
	readonly DevObject objectToAdd;
	readonly Room room;
	public ObjectAddChange(Room room, DevObject devObject, bool adding) {
		this.objectToAdd = devObject;
		this.adding = adding;
		this.room = room;
	}

	private void Add() {
		this.room.data.objects.Add(this.objectToAdd);
	}

	private void Remove() {
		this.room.data.objects.Remove(this.objectToAdd);
	}

	public override void Redo() {
		if (this.adding)
			this.Add();
		else
			this.Remove();
	}

	public override void Undo() {
		if (this.adding)
			this.Remove();
		else
			this.Add();
	}
}