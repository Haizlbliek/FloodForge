namespace FloodForge.World;

public class RoomAndConnectionChange : Change {
	protected readonly bool adding;
	protected readonly List<Room> rooms = [];
	protected readonly List<Connection> externalConnections = []; // on one side connected to a removed room
	protected readonly List<Connection> internalConnections = []; // on both sides connected to a removed room

	public RoomAndConnectionChange(bool adding) {
		this.adding = adding;
	}

	public void AddRoom(Room room) {
		foreach (Connection connection in room.connections) {
			if(this.rooms.Contains(connection.roomA) || this.rooms.Contains(connection.roomB))
				this.internalConnections.Add(connection);
		}
		this.rooms.Add(room);
	}

	public void AddConnection(Connection connection) => this.externalConnections.Add(connection);

	protected void Add() {
		foreach (Room room in this.rooms) {
			if (room is OffscreenRoom) continue;

			// LATER: Add into correct index
			WorldWindow.region.rooms.Add(room);
		}

		foreach (Connection internalConnection in this.internalConnections) {
			WorldWindow.region.connections.Add(internalConnection);
		}

		foreach (Connection connection in this.externalConnections) {
			// LATER: Add into correct index
			WorldWindow.region.connections.Add(connection);
			connection.roomA.Connect(connection);
			connection.roomB.Connect(connection);
		}
	}

	protected void Remove() {
		foreach (Connection connection in this.externalConnections) {
			connection.roomA.Disconnect(connection);
			connection.roomB.Disconnect(connection);
			WorldWindow.region.connections.Remove(connection);
		}

		foreach (Connection internalConnection in this.internalConnections) {
			WorldWindow.region.connections.Remove(internalConnection);
		}

		foreach (Room room in this.rooms) {
			if (room is OffscreenRoom) continue;

			WorldWindow.region.rooms.Remove(room);
		}
	}

	public override void Undo() {
		if (this.adding) {
			this.Remove();
		}
		else {
			this.Add();
		}
	}

	public override void Redo() {
		if (this.adding) {
			this.Add();
		}
		else {
			this.Remove();
		}
	}
}