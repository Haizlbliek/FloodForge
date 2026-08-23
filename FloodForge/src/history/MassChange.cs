namespace FloodForge.History;

public class MassChange : Change {
	readonly Change[] changes;
	readonly Action? callback;
	protected MassChange[] mergedChanges = [];
	public MassChange(Change[] changes, Action? callback = null) {
		this.changes = changes;
		this.callback = callback;
	}

	public override void Redo() {
		foreach (Change change in this.changes) {
			change.Redo();
		}
		this.callback?.Invoke();
		foreach (MassChange massChange in this.mergedChanges) {
			massChange.Redo();
		}
	}

	public override void Undo() {
		foreach (MassChange massChange in this.mergedChanges.Reverse()) {
			massChange.Undo();
		}
		foreach (Change change in this.changes.Reverse()) {
			change.Undo();
		}
		this.callback?.Invoke();
	}

	public int GetCount() {
		return this.changes.Length;
	}
	
	/// <summary>
	/// DOES NOT PRESERVE CALLBACK ORDER!
	/// </summary>
	public void Merge(MassChange massChange) {
		List<MassChange> mergedChanges = [.. this.mergedChanges];
		mergedChanges.Add(massChange);
		this.mergedChanges = [.. mergedChanges];
	}
}