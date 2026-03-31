namespace FloodForge.World;

public class MoveChange : MultipleRoomChange {
	protected readonly List<Vector2> devOffsets = [];
	protected readonly List<Vector2> canonOffsets = [];

	[Obsolete("Use AddRoom(Room, Vector2, Vector2) instead")]
	public new void AddRoom(Room room) {
		throw new NotSupportedException("Use AddRoom(Room, Vector2, Vector2) instead");
	}

	public virtual void AddRoom(Room room, Vector2 devOffset, Vector2 canonOffset) {
		base.AddRoom(room);
		this.devOffsets.Add(devOffset);
		this.canonOffsets.Add(canonOffset);
	}

	public void Merge(MoveChange other) {
		for (int i = 0; i < other.rooms.Count; i++) {
			int j = this.rooms.IndexOf(other.rooms[i]);

			if (j == -1) {
				this.AddRoom(other.rooms[i], other.devOffsets[i], other.canonOffsets[i]);
			}
			else {
				this.devOffsets[j] += other.devOffsets[i];
				this.canonOffsets[j] += other.canonOffsets[i];
			}
		}
	}

	protected void Move(float multiplier) {
		if (Main.AprilFools) {
			for (int i = 0; i < this.rooms.Count; i++) {
				this.rooms[i].DevVel += this.devOffsets[i] * multiplier;
				this.rooms[i].CanonVel += this.canonOffsets[i] * multiplier;
			}
		}
		else {
			for (int i = 0; i < this.rooms.Count; i++) {
				this.rooms[i].DevPosition += this.devOffsets[i] * multiplier;
				this.rooms[i].CanonPosition += this.canonOffsets[i] * multiplier;
			}
		}
	}

	public override void Undo() {
		this.Move(-1f);
	}

	public override void Redo() {
		this.Move(1f);
	}
}
