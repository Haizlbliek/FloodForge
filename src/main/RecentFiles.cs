using System.IO;
using Stride.Core.Extensions;

namespace FloodForge.World;

// TODO: Fix the backwards-ness
public static class RecentFiles {
	public static List<string> recents = [];
	public static List<string> recentNames = [];

	public static string GetRegionDisplayname(string worldPath) {
		string? displaynamePath = PathUtil.FindFile(PathUtil.Parent(worldPath), "displayname.txt");

		return displaynamePath == null ? "" : File.ReadAllText(displaynamePath).Trim();
	}

	public static void Initialize() {
		string recentsPath = "assets/recents.txt";
		if (!File.Exists(recentsPath)) return;

		foreach (string path in File.ReadAllLines(recentsPath)) {
			if (path.IsNullOrEmpty()) continue;
			if (!File.Exists(path)) continue;

			recents.Add(path);
			recentNames.Add(GetRegionDisplayname(path));
		}
	}

	public static void AddPath(string path) {
		int i = recents.FindIndex(x => x.ToLowerInvariant() == path.ToLowerInvariant());
		string? name = null;
		if (i != -1) {
			recents.RemoveAt(i);
			name = recentNames[i];
			recentNames.RemoveAt(i);
		}
		name ??= GetRegionDisplayname(path);
		recents.Add(path);
		recentNames.Add(name);
		Save();
	}

	private static void Save() {
		File.WriteAllText("assets/recents.txt", string.Join('\n', recents));
	}
}