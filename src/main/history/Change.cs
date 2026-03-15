namespace FloodForge.World;

public abstract class Change {
	public abstract void Undo();
	public abstract void Redo();
}