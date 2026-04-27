using FloodForge.World;

namespace FloodForge.History;

public class AttractivenessChange : MultipleRoomChange {
	protected RoomAttractiveness attr;
	protected string creature;
	protected (bool, RoomAttractiveness) undoAttr;
	protected List<(bool, RoomAttractiveness)> undoAttrs = [];

	public AttractivenessChange(RoomAttractiveness attr, string creature) {
		this.attr = attr;
		this.creature = creature;
		if (WorldWindow.region.defaultAttractiveness.TryGetValue(this.creature, out RoomAttractiveness attractiveness)) {
			this.undoAttr = (true, attractiveness);
		}
		else {
			this.undoAttr = (false, RoomAttractiveness.Default);
		}
	}

	public override void AddRoom(Room room) {
		base.AddRoom(room);
		if (room.data.attractiveness.TryGetValue(this.creature, out RoomAttractiveness attractiveness)) {
			this.undoAttr = (true, attractiveness);
		}
		else {
			this.undoAttr = (false, RoomAttractiveness.Default);
		}
	}

	public override void Undo() {
		if (this.rooms.Count == 0) {
			if (this.undoAttr.Item1) {
				WorldWindow.region.defaultAttractiveness[this.creature] = this.undoAttr.Item2;
			}
			else {
				WorldWindow.region.defaultAttractiveness.Remove(this.creature);
			}
		}
		else {
			for (int i = 0; i < this.rooms.Count; i++) {
				if (this.undoAttrs[i].Item1) {
					this.rooms[i].data.attractiveness[this.creature] = this.undoAttrs[i].Item2;
				}
				else {
					this.rooms[i].data.attractiveness.Remove(this.creature);
				}
			}
		}
	}

	public override void Redo() {
		if (this.attr == RoomAttractiveness.Default) {
			if (this.rooms.Count == 0) {
				WorldWindow.region.defaultAttractiveness.Remove(this.creature);
			}
			else {
				foreach (Room room in this.rooms) {
					room.data.attractiveness.Remove(this.creature);
				}
			}
		}
		else {
			if (this.rooms.Count == 0) {
				WorldWindow.region.defaultAttractiveness[this.creature] = this.attr;
			}
			else {
				foreach (Room room in this.rooms) {
					room.data.attractiveness[this.creature] = this.attr;
				}
			}
		}
	}
}