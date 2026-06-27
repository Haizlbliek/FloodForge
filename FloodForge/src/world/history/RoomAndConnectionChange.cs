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
			// - if we add a room that replaces another room, add this room to that room's replacingrooms
			// - if we add a room that is replaced by another room, set that room's replacedroom to this room
			if (room.replacedRoom != null && WorldWindow.region.rooms.Contains(room.replacedRoom) && !this.rooms.Contains(room.replacedRoom)) {
				room.replacedRoom.replacingRooms.Add(room);
				room.replacedRoom.MoveUpdate();
			}
			foreach (Room replacingRoom in room.replacingRooms) {
				replacingRoom.replacedRoom = room;
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
			// if a room replaces another room, remove this room from that room's replacingrooms
			if (room.replacedRoom != null && WorldWindow.region.rooms.Contains(room.replacedRoom) && !this.rooms.Contains(room.replacedRoom)) {
				room.replacedRoom.replacingRooms.Remove(room);
				room.replacedRoom.MoveUpdate();
			}
			// if a room is replaced by another room, remove this room from its replacedroomness
			foreach (Room replacingRoom in room.replacingRooms) {
				replacingRoom.replacedRoom = null;
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