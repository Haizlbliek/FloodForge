namespace FloodForge.World;

public class MoveChange : MultipleDraggableChange {
	protected readonly List<Vector2> devOffsets = [];
	protected readonly List<Vector2> canonOffsets = [];

	public virtual void AddDraggable(WorldDraggable draggable, Vector2 devOffset, Vector2 canonOffset) {
		base.AddDraggable(draggable);
		this.devOffsets.Add(devOffset);
		this.canonOffsets.Add(canonOffset);
	}

	public void Merge(MoveChange other) {
		for (int i = 0; i < other.draggables.Count; i++) {
			int j = this.draggables.IndexOf(other.draggables[i]);

			if (j == -1) {
				this.AddDraggable(other.draggables[i], other.devOffsets[i], other.canonOffsets[i]);
			}
			else {
				this.devOffsets[j] += other.devOffsets[i];
				this.canonOffsets[j] += other.canonOffsets[i];
			}
		}
	}

	protected void Move(float multiplier) {
		for (int i = 0; i < this.draggables.Count; i++) {
			if(this.draggables[i] is Room room) {
				room.DevPosition += this.devOffsets[i] * multiplier;
				room.CanonPosition += this.canonOffsets[i] * multiplier;
				room.MoveUpdate();
			}
			else {
				this.draggables[i].Position += this.devOffsets[i] * multiplier;
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
