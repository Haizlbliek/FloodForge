using FloodForge.World;

namespace FloodForge.History;

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

	public Room[] GetRooms() {
		return [..this.rooms];
	}

	public void AddConnection(Connection connection) => this.externalConnections.Add(connection);

	public Connection[] GetInternalConnections() {
		return [.. this.internalConnections];
	}
	public Connection[] GetExternalConnections() {
		return [.. this.externalConnections];
	}

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

			ConnectionVisual? visual = null;
			foreach (RoomReplacement replacement in connection.roomA.roomReplacements) {
				if (replacement.replacedRoom == connection.roomA) {
					visual = new (replacement.replacingRoom, connection.roomB, connection.roomAExitID, connection.roomBExitID);
					connection.replacementVirtualConnections.Add(visual);
					WorldWindow.virtualConnections.Add(visual);
				}
			}
			foreach (RoomReplacement replacement in connection.roomB.roomReplacements) {
				if (replacement.replacedRoom == connection.roomB) {
					visual = new (connection.roomA, replacement.replacingRoom, connection.roomAExitID, connection.roomBExitID);
					connection.replacementVirtualConnections.Add(visual);
					WorldWindow.virtualConnections.Add(visual);
				}
			}
		}
	}

	protected void Remove() {
		foreach (Connection connection in this.externalConnections) {
			connection.roomA.Disconnect(connection);
			connection.roomB.Disconnect(connection);
			WorldWindow.region.connections.Remove(connection);
			connection.replacementVirtualConnections.ForEach(x => WorldWindow.virtualConnections.Remove(x));
		}

		foreach (Connection internalConnection in this.internalConnections) {
			WorldWindow.region.connections.Remove(internalConnection);
			internalConnection.replacementVirtualConnections.ForEach(x => WorldWindow.virtualConnections.Remove(x));
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