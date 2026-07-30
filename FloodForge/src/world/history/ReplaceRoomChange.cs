using FloodForge.World;

namespace FloodForge.History;

/// <summary>
/// Not to be confused with RoomReplacementChange. This one's for REPLACEROOM adding/removal.
/// </summary>
public class ReplaceRoomChange : Change {
	private bool adding;
	private List<ReplaceRoom> affectedReplaceRooms;
	private int index = -1;
	public ReplaceRoomChange(bool adding) {
		this.adding = adding;
		this.affectedReplaceRooms = [];
	}

	public void AddReplaceRoom(ReplaceRoom replaceRoom) {
		this.affectedReplaceRooms.Add(replaceRoom);
	}

	private void Add() {
		foreach (ReplaceRoom affectedReplaceRoom in this.affectedReplaceRooms) {
			int indexToUse = this.index;
			if (this.index == -1) {
				indexToUse = WorldWindow.replaceRooms.Count;
			}
			affectedReplaceRoom.replacedRoom.replaceRooms.Add(affectedReplaceRoom);
			if (affectedReplaceRoom.replacingRoom.referencingReplaceRooms.Count == 0 && affectedReplaceRoom.replacingRoom.isVirtualRoom) {
				WorldWindow.replaceReferenceRooms.Add(affectedReplaceRoom.replacingRoom);
			}
			affectedReplaceRoom.replacingRoom.referencingReplaceRooms.Add(affectedReplaceRoom);
			WorldWindow.replaceRooms.Insert(indexToUse, affectedReplaceRoom);
		}
	}

	private void Remove() {
		foreach (ReplaceRoom affectedReplaceRoom in this.affectedReplaceRooms.AsEnumerable().Reverse()) {
			this.index = WorldWindow.replaceRooms.IndexOf(affectedReplaceRoom);
			affectedReplaceRoom.replacedRoom.replaceRooms.Remove(affectedReplaceRoom);
			affectedReplaceRoom.replacingRoom.referencingReplaceRooms.Remove(affectedReplaceRoom);
			if (affectedReplaceRoom.replacingRoom.referencingReplaceRooms.Count == 0) {
				WorldWindow.replaceReferenceRooms.Remove(affectedReplaceRoom.replacingRoom);
			}
			WorldWindow.replaceRooms.Remove(affectedReplaceRoom);
		}
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