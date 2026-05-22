namespace FloodForge.History;

public abstract class Change {
	public abstract void Undo();
	public abstract void Redo();
}