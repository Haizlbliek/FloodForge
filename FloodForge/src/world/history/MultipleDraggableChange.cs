namespace FloodForge.World;

public abstract class MultipleDraggableChange : Change {
	protected readonly List<WorldDraggable> draggables = [];

	public virtual void AddDraggable(WorldDraggable draggable) {
		this.draggables.Add(draggable);
	}
}