namespace FloodForge.World;

public abstract class MultipleRoomChange : Change {
	protected readonly List<Room> rooms = [];

	public virtual void AddRoom(Room room) {
		this.rooms.Add(room);
	}
}