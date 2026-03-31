using Stride.Core.Extensions;

namespace FloodForge.World;

public static class RecentFiles {
	public static List<string> recents = [];
	public static List<string> recentNames = [];

	public static void Initialize() {
		string recentsPath = "assets/recents.txt";
		if (!File.Exists(recentsPath)) return;

		foreach (string path in File.ReadAllLines(recentsPath)) {
			if (path.IsNullOrEmpty()) continue;
			if (!File.Exists(path)) continue;

			recents.Add(path);
			recentNames.Add(WorldParser.GetRegionDisplayname(path));
		}
	}

	public static void AddPath(string path) {
		int i = recents.FindIndex(x => x.Equals(path, StringComparison.InvariantCultureIgnoreCase));
		string? name = null;
		if (i != -1) {
			recents.RemoveAt(i);
			name = recentNames[i];
			recentNames.RemoveAt(i);
		}
		name ??= WorldParser.GetRegionDisplayname(path);
		recents.Insert(0, path);
		recentNames.Insert(0, name);
		Save();
	}

	private static void Save() {
		File.WriteAllText("assets/recents.txt", string.Join('\n', recents));
	}
}