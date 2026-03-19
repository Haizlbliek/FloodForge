using Stride.Core.Extensions;

namespace FloodForge.World;

public static class CreatureTextures {
	public const string CLEAR = "clear";
	public const string UNKNOWN = "unknown";

	private static readonly Dictionary<string, Texture> creatureTextures = [];
	private static readonly Dictionary<string, Texture> creatureTagTextures = [];
	public static readonly List<string> creatures = [];
	public static readonly List<string> creatureOrder = [];
	public static readonly List<string> creatureTags = [];

	private static readonly Dictionary<string, string> parseMap = [];
	public static readonly Dictionary<string, string> exportCreatureNames = [];

	public static Texture UnknownCreature { get; private set; } = null!;

	private static void LoadCreaturesFromFolder(string path) {
		Logger.Info("Loading creatures from: " + path);

		foreach (string file in Directory.EnumerateFiles(path)) {
			if (!file.EndsWith(".png")) continue;

			string creature = Path.GetFileNameWithoutExtension(file).ToLowerInvariant();
			creatures.Add(creature);
			creatureTextures[creature] = Texture.Load(file, TextureWrapMode.ClampToBorder);
			exportCreatureNames[creature] = Path.GetFileNameWithoutExtension(file);
		}

		string parse = Path.Combine(path, "parse.txt");
		if (File.Exists(parse)) {
			foreach (string line in File.ReadAllLines(parse)) {
				if (line.IsNullOrEmpty()) continue;

				int idx = line.IndexOf('>');
				string from = line[..idx].ToLowerInvariant();
				string to = line[(idx + 1)..].ToLowerInvariant();

				parseMap[from] = to;
			}
		}

		string order = Path.Combine(path, "order.txt");
		if (File.Exists(order)) {
			foreach (string line in File.ReadAllLines(order)) {
				if (line.IsNullOrEmpty()) continue;
				if (line.StartsWith("//")) continue;

				creatureOrder.Add(line.ToLowerInvariant().Trim());
			}
		}
	}

	private static void LoadRoomItemsFromFolder(string path) {
		// LATER Don't store these in CreatureTextures
		Logger.Info("Loading room items from: " + path);

		foreach (string file in Directory.EnumerateFiles(path)) {
			if (!file.EndsWith(".png")) continue;

			string item = "room-" + Path.GetFileNameWithoutExtension(file).ToLowerInvariant();
			creatureTextures[item] = Texture.Load(file);
		}
	}

	public static void Initialize() {
		string creaturesDirectory = "assets/creatures";
		string modsPath = Path.Combine(creaturesDirectory, "mods.txt");
		if (!File.Exists(modsPath)) return;

		string[] mods = File.ReadAllLines(modsPath);

		creatureOrder.Add(CLEAR);
		LoadCreaturesFromFolder(creaturesDirectory);
		foreach (string mod in mods) {
			if (mod.IsNullOrEmpty()) continue;

			LoadCreaturesFromFolder(Path.Combine(creaturesDirectory, mod));
		}
		LoadRoomItemsFromFolder(Path.Combine(creaturesDirectory, "room"));

		foreach (string path in Directory.EnumerateFiles(Path.Combine(creaturesDirectory, "TAGS"))) {
			if (!path.EndsWith(".png")) continue;

			string tag = Path.GetFileNameWithoutExtension(path);
			creatureTags.Add(tag);
			creatureTagTextures[tag] = Texture.Load(path);
		}

		int idx = creatures.IndexOf(CLEAR);
		(creatures[idx], creatures[0]) = (creatures[0], creatures[idx]);

		idx = creatures.IndexOf(UNKNOWN);
		UnknownCreature = creatureTextures[UNKNOWN];
		(creatures[idx], creatures[^1]) = (creatures[^1], creatures[idx]);

		foreach (string creature in creatures) {
			if (creature == CLEAR || creature == UNKNOWN) continue;
			if (creatureOrder.Contains(creature)) continue;

			creatureOrder.Add(creature);
		}

		creatureOrder.Add(UNKNOWN);
	}

	public static Texture GetTexture(string type, bool lowercase = true) {
		if (lowercase) type = type.ToLowerInvariant();

		if (creatureTagTextures.TryGetValue(type, out Texture? tex)) {
			return tex;
		}

		if (creatureTextures.TryGetValue(type, out tex)) {
			return tex;
		}

		return UnknownCreature;
	}

	public static string Parse(string type) {
		type = type.ToLowerInvariant();

		if (type == "NONE") return "";

		if (parseMap.TryGetValue(type, out string? o)) {
			return o;
		}

		return type;
	}

	public static bool Known(string type) {
		if (type.IsNullOrEmpty()) return true;

		return creatureTextures.ContainsKey(Parse(type));
	}
}