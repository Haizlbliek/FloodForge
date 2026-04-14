namespace FloodForge.World;

public static class History {
	private static readonly Stack<Change> undos = [];
	private static readonly Stack<Change> redos = [];

	public static Change? Last => undos.Count == 0 ? null : undos.Peek();

	private static bool Collectingchanges = false;
	private static List<Change> collectedChanges = [];
	private static List<Type> typesToCollect = [];
	
	/// <summary>
	/// Start collecting changes of types <c>collectingTypes</c> instead of applying them directly.
	/// Does not yet support multiple differing collections happening at the same time.
	/// Make sure to use <c>StopCollectingChanges</c> in order to avoid nullifying all changes of types <c>collectingTypes</c>.
	/// </summary>
	public static void StartCollectingChanges(List<Type> collectingTypes) {
		if(Collectingchanges) throw new ("Cannot start new collection while collection is in progress!");
		collectedChanges = [];
		Collectingchanges = true;
		typesToCollect = collectingTypes;
	}
	
	
	/// <summary>
	/// Stop collecting changes and return an array of all collected changes.
	/// Of note: this does not automatically apply the collected changes. Apply them after if necessary.
	/// Does not yet support multiple differing collections happening at the same time.
	/// </summary>
	public static Change[] StopCollectingChanges() {
		Collectingchanges = false;
		return [.. History.collectedChanges];
	}

	/// <summary>
	/// Stop collecting changes and return a new MassChange from all collected changes.
	/// Does not yet support multiple differing collections happening at the same time.
	/// </summary>
	public static MassChange GetCollectedMassChange() {
		return new MassChange(StopCollectingChanges());
	}

	public static void Apply(Change change) {
		if (!Collectingchanges || (typesToCollect.Count != 0 && !typesToCollect.Contains(change.GetType()))) {
			change.Redo();

			redos.Clear();
			undos.Push(change);
		}
		else {
			collectedChanges.Add(change);
		}
	}

	public static void Clear() {
		redos.Clear();
		undos.Clear();
	}

	public static void Undo() {
		if (undos.Count == 0) return;
		Change change = undos.Pop();
		change.Undo();
		redos.Push(change);
	}

	public static void Redo() {
		if (redos.Count == 0) return;
		Change change = redos.Pop();
		change.Redo();
		undos.Push(change);
	}
}