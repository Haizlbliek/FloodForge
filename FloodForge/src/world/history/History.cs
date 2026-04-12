namespace FloodForge.World;

public static class History {
	private static readonly Stack<Change> undos = [];
	private static readonly Stack<Change> redos = [];

	public static Change? Last => undos.Count == 0 ? null : undos.Peek();

	private static bool Collectingchanges = false;
	private static List<Change> collectedChanges = [];
	private static List<Type> typesToCollect = [];
	public static void StartCollectingChanges(List<Type> collectingTypes) {
		Collectingchanges = true;
		typesToCollect = collectingTypes;
	}
	
	public static void StopCollectingChanges(out Change[] collectedChanges) {
		Collectingchanges = false;
		collectedChanges = [.. History.collectedChanges];
		History.collectedChanges = [];
		return;
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