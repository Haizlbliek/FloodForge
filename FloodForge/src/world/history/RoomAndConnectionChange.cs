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
			foreach (RoomReplacement roomReplacement in room.roomReplacements) {
				if (roomReplacement.replacedRoom == room && WorldWindow.region.rooms.Contains(roomReplacement.replacingRoom) && !this.rooms.Contains(roomReplacement.replacingRoom))
					roomReplacement.replacingRoom.roomReplacements.Add(roomReplacement);
				else if (roomReplacement.replacingRoom == room && WorldWindow.region.rooms.Contains(roomReplacement.replacedRoom) && !this.rooms.Contains(roomReplacement.replacedRoom)) {
					roomReplacement.replacedRoom.roomReplacements.Add(roomReplacement);
					roomReplacement.replacedRoom.MoveUpdate();
				}
			}
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
			foreach (RoomReplacement roomReplacement in room.roomReplacements) {
				if (roomReplacement.replacedRoom == room && WorldWindow.region.rooms.Contains(roomReplacement.replacingRoom) && !this.rooms.Contains(roomReplacement.replacingRoom))
					roomReplacement.replacingRoom.roomReplacements.Remove(roomReplacement);
				else if (roomReplacement.replacingRoom == room && WorldWindow.region.rooms.Contains(roomReplacement.replacedRoom) && !this.rooms.Contains(roomReplacement.replacedRoom)) {
					roomReplacement.replacedRoom.roomReplacements.Remove(roomReplacement);
					roomReplacement.replacedRoom.MoveUpdate();
				}
			}
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