using FloodForge.World;

namespace FloodForge.History;

public abstract class MultipleDraggableChange : Change {
	protected readonly List<WorldDraggable> draggables = [];

	public virtual void AddDraggable(WorldDraggable draggable) {
		this.draggables.Add(draggable);
	}
}