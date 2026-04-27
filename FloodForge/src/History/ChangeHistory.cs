namespace FloodForge.History;

public class ChangeHistory {
	private readonly Stack<Change> undos = [];
	private readonly Stack<Change> redos = [];

	public Change? Last => this.undos.Count == 0 ? null : this.undos.Peek();
	private readonly bool RedoChangeOnApply = true;

	private bool Collectingchanges = false;
	private List<Change> collectedChanges = [];
	private List<Type> typesToCollect = [];

	public ChangeHistory(bool RedoChangeOnApply = true) {
		this.RedoChangeOnApply = RedoChangeOnApply;
	}
	
	/// <summary>
	/// Start collecting changes of types <c>collectingTypes</c> instead of applying them directly.
	/// Does not yet support multiple differing collections happening at the same time.
	/// Make sure to use <c>StopCollectingChanges</c> in order to avoid nullifying all changes of types <c>collectingTypes</c>.
	/// </summary>
	public void StartCollectingChanges(List<Type> collectingTypes) {
		if(this.Collectingchanges) throw new ("Cannot start new collection while collection is in progress!");
		this.collectedChanges = [];
		this.Collectingchanges = true;
		this.typesToCollect = collectingTypes;
	}
	
	
	/// <summary>
	/// Stop collecting changes and return an array of all collected changes.
	/// Of note: this does not automatically apply the collected changes. Apply them after if necessary.
	/// Does not yet support multiple differing collections happening at the same time.
	/// </summary>
	public Change[] StopCollectingChanges() {
		this.Collectingchanges = false;
		return [.. this.collectedChanges];
	}

	/// <summary>
	/// Stop collecting changes and return a new MassChange from all collected changes.
	/// Does not yet support multiple differing collections happening at the same time.
	/// </summary>
	public MassChange GetCollectedMassChange() {
		return new MassChange(this.StopCollectingChanges());
	}

	public void Apply(Change change) {
		if (!this.Collectingchanges || (this.typesToCollect.Count != 0 && !this.typesToCollect.Contains(change.GetType()))) {
			if(this.RedoChangeOnApply) change.Redo();

			this.redos.Clear();
			this.undos.Push(change);
		}
		else {
			this.collectedChanges.Add(change);
		}
	}

	public void Clear() {
		this.redos.Clear();
		this.undos.Clear();
	}

	public void Undo() {
		if (this.undos.Count == 0) return;
		Change change = this.undos.Pop();
		change.Undo();
		this.redos.Push(change);
	}

	public void Redo() {
		if (this.redos.Count == 0) return;
		Change change = this.redos.Pop();
		change.Redo();
		this.undos.Push(change);
	}
}