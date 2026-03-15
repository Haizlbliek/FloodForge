using System.IO;

namespace FloodForge.World;

public static class ConditionalTimelineTextures {
	public const string UNKNOWN = "UNKNOWN";
	public const string WARNING = "WARNING";
	public static Texture UnknownCreature { get; private set; } = null!;
	public static Texture WarningCreature { get; private set; } = null!;

	public static readonly Dictionary<string, Texture> textures = [];
	public static readonly List<string> timelines = [];

	public static void Initialize() {
		string path = "assets/timelines";
		Logger.Info($"Loading timelines from {path}");
		foreach (string file in Directory.EnumerateFiles(path)) {
			if (!file.EndsWith(".png")) continue;

			string creature = Path.GetFileNameWithoutExtension(file);
			timelines.Add(creature);
			textures[creature] = Texture.Load(file);

			if (creature == UNKNOWN) UnknownCreature = textures[creature];
			if (creature == WARNING) UnknownCreature = textures[creature];
		}

		timelines.Remove(UNKNOWN);
		timelines.Remove(WARNING);
	}

	public static Texture GetTexture(string timeline) {
		if (textures.TryGetValue(timeline, out Texture? tex)) {
			return tex;
		}

		return UnknownCreature;
	}

	public static bool HasTimeline(string timeline) {
		return timelines.Contains(timeline);
	}
}