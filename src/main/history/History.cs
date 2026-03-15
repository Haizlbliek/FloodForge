namespace FloodForge.World;

public static class History {
	private static readonly Stack<Change> undos = [];
	private static readonly Stack<Change> redos = [];

	public static Change? Last => undos.Count == 0 ? null : undos.Peek();

	public static void Apply(Change change) {
		change.Redo();

		redos.Clear();
		undos.Push(change);
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