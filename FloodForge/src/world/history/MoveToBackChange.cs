using FloodForge.World;

namespace FloodForge.History;

public class MoveToBackChange : Change {
	protected List<(Room room, int index)> originalStates;

	public MoveToBackChange(IEnumerable<Room> rooms) {
		this.originalStates = [.. rooms
			.Select(r => (room: r, index: WorldWindow.region.rooms.IndexOf(r)))
			.OrderBy(state => state.index)];
	}

	public override void Undo() {
		foreach ((Room? room, int _) in this.originalStates) {
			WorldWindow.region.rooms.Remove(room);
		}

		foreach ((Room? room, int index) in this.originalStates) {
			WorldWindow.region.rooms.Insert(index, room);
		}
	}

	public override void Redo() {
		foreach ((Room? room, int _) in this.originalStates) {
			WorldWindow.region.rooms.Remove(room);
		}

		for (int i = this.originalStates.Count - 1; i >= 0; i--) {
			WorldWindow.region.rooms.Insert(0, this.originalStates[i].room);
		}
	}
}
