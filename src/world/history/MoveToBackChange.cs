namespace FloodForge.World;

public class MoveToBackChange : Change {
	protected List<(Room room, int index)> originalStates;

	public MoveToBackChange(IEnumerable<Room> rooms) {
		this.originalStates = [.. rooms
			.Select(r => (room: r, index: WorldWindow.region.rooms.IndexOf(r)))
			.OrderBy(state => state.index)];
	}

	public override void Undo() {
		foreach (var (room, _) in this.originalStates) {
			WorldWindow.region.rooms.Remove(room);
		}

		foreach (var (room, index) in this.originalStates) {
			WorldWindow.region.rooms.Insert(index, room);
		}
	}

	public override void Redo() {
		foreach (var (room, _) in this.originalStates) {
			WorldWindow.region.rooms.Remove(room);
		}

		for (int i = this.originalStates.Count - 1; i >= 0; i--) {
			WorldWindow.region.rooms.Insert(0, this.originalStates[i].room);
		}
	}
}
