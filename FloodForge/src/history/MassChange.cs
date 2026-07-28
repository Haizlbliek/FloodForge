namespace FloodForge.History;

public class MassChange : Change {
	readonly Change[] changes;
	readonly Action? callback;
	public MassChange(Change[] changes, Action? callback = null) {
		this.changes = changes;
		this.callback = callback;
	}

	public override void Redo() {
		foreach (Change change in this.changes) {
			change.Redo();
		}
		this.callback?.Invoke();
	}

	public override void Undo() {
		foreach (Change change in this.changes.Reverse()) {
			change.Undo();
		}
		this.callback?.Invoke();
	}

	public int GetCount() {
		return this.changes.Length;
	}
}