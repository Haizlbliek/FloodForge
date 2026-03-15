namespace FloodForge.World;

public class RoomAndConnectionChange : Change {
	protected readonly bool adding;
	protected readonly List<Room> rooms = [];
	protected readonly List<Connection> connections = [];

	public RoomAndConnectionChange(bool adding) {
		this.adding = adding;
	}

	public void AddRoom(Room room) => this.rooms.Add(room);

	public void AddConnection(Connection connection) => this.connections.Add(connection);

	protected void Add() {
		foreach (Room room in this.rooms) {
			if (room is OffscreenRoom) continue;

			// LATER: Add into correct index
			WorldWindow.region.rooms.Add(room);
		}

		foreach (Connection connection in this.connections) {
			// LATER: Add into correct index
			WorldWindow.region.connections.Add(connection);
			connection.roomA.Connect(connection);
			connection.roomB.Connect(connection);
		}
	}

	protected void Remove() {
		foreach (Connection connection in this.connections) {
			connection.roomA.Disconnect(connection);
			connection.roomB.Disconnect(connection);
			WorldWindow.region.connections.Remove(connection);
		}

		foreach (Room room in this.rooms) {
			if (room is OffscreenRoom) continue;

			WorldWindow.region.rooms.Remove(room);
		}
	}

	public override void Undo() {
		if (this.adding) {
			this.Remove();
		} else {
			this.Add();
		}
	}

	public override void Redo() {
		if (this.adding) {
			this.Add();
		} else {
			this.Remove();
		}
	}
}