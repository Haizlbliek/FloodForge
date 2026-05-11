using System.Globalization;
using System.Text.RegularExpressions;
using FloodForge.World;
using Stride.Core.Extensions;

namespace FloodForge;

public static class Mods {
	public static Texture Clear { get; private set; } = null!;
	public static Texture Unknown { get; private set; } = null!;
	public static Texture Warning { get; private set; } = null!;

	public static readonly Dictionary<string, Texture> creatureTextures = [];
	public static readonly List<string> creatures = [ "CLEAR" ];
	private static readonly Dictionary<string, string> creatureParseMap = [];
	private static readonly Dictionary<string, string> creatureExportNames = [];

	public static readonly Dictionary<string, string> tagExportNames = [];
	public static readonly Dictionary<string, Texture> tagTextures = [];
	public static readonly Dictionary<string, Tag> tags = [];

	public static readonly Dictionary<string, Texture> timelineTextures = [];
	public static readonly List<string> timelines = [];

	public static readonly Dictionary<string, Texture> objectTextures = [];

	public static IEnumerable<Tag> Tags(string creature) => tags.Values.Where(t => t.Supports(creature));

	public static void Initialize() {
		string modsPath = "assets/mods.txt";
		if (!File.Exists(modsPath)) return;

		Clear = Texture.Load("assets/icons/CLEAR.png", TextureWrapMode.ClampToBorder);
		Unknown = Texture.Load("assets/icons/UNKNOWN.png", TextureWrapMode.ClampToBorder);
		Warning = Texture.Load("assets/icons/WARNING.png", TextureWrapMode.ClampToBorder);

		string[] mods = File.ReadAllLines(modsPath);

		Dictionary<string, HashSet<string>> tagVariables = new Dictionary<string, HashSet<string>>(StringComparer.InvariantCultureIgnoreCase);
		Dictionary<string, (DisplayType, List<string>)> tagData = new Dictionary<string, (DisplayType, List<string>)>(StringComparer.InvariantCultureIgnoreCase);

		foreach (string modName in mods) {
			if (modName.IsNullOrEmpty()) continue;

			string? dir = PathUtil.FindDirectory("assets/mods", modName);
			if (dir == null) continue;

			if (PathUtil.TryGetDirectory(dir, "creatures", out string? creaturesPath)) {
				foreach (string filePath in Directory.GetFiles(creaturesPath, "*.png")) {
					string creature = Path.GetFileNameWithoutExtension(filePath);
					creatureTextures.Add(creature.ToLowerInvariant(), Texture.Load(filePath, TextureWrapMode.ClampToBorder));
					creatureExportNames.Add(creature.ToLowerInvariant(), creature);
				}

				if (PathUtil.TryGetFile(creaturesPath, "order.txt", out string? file)) {
					creatures.AddRange(File.ReadAllText(file).Split("\n", StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries).Select(x => x.ToLowerInvariant()));
				}

				if (PathUtil.TryGetFile(creaturesPath, "parse.txt", out file)) {
					File.ReadAllText(file)
					.Split("\n", StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
					.Select(x => x.Split('>', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
					.ForEach(x => {
						creatureParseMap.Add(x[0].ToLowerInvariant(), x[1].ToLowerInvariant());
					});
				}
			}

			if (PathUtil.TryGetDirectory(dir, "tags", out string? tagsPath)) {
				foreach (string filePath in Directory.GetFiles(tagsPath, "*.png")) {
					string tag = Path.GetFileNameWithoutExtension(filePath);
					tagTextures.Add(tag.ToLowerInvariant(), Texture.Load(filePath));
					tagExportNames.Add(tag.ToLowerInvariant(), tag);
				}

				if (PathUtil.TryGetFile(tagsPath, "tags.txt", out string? file)) {
					string? workingTag = null;
					int tagLine = 0;
					string tagType = "";
					List<string> tagCreatures = [];
					DisplayType? displayType = null;

					string[] lines = File.ReadAllLines(file);
					for (int i = 0; i < lines.Length; i++) {
						string line = lines[i].Trim().ToLowerInvariant();
						if (line.IsNullOrEmpty()) continue;

						if (line.StartsWith("{#")) {
							Match match = Regex.Match(line, @"\{#(?<name>\w+)\s*=\s*(?:(?<item>~?\w+)(?:,\s*)?)*\}");
							if (match.Success) {
								string varName = match.Groups["name"].Value;
								if (!tagVariables.TryGetValue(varName, out HashSet<string>? var)) {
									var = new HashSet<string>(StringComparer.InvariantCultureIgnoreCase);
									tagVariables[varName] = var;
								}
								foreach (Capture capture in match.Groups["item"].Captures) {
									string val = capture.Value;
									bool isRemove = val.StartsWith('~');
									string creature = isRemove ? val[1..] : val;
									if (isRemove) {
										var.Remove(creature);
									}
									else {
										var.Add(creature);
									}
								}
							}
						}
						else if (line.StartsWith('[')) {
							if (workingTag != null && displayType != null) {
								tagData.Add(workingTag, (displayType, tagCreatures));
							}
							workingTag = line[1..^1];
							displayType = null;
							tagLine = 0;
							tagType = "";
							tagCreatures = [];
						}
						else {
							tagLine++;
							if (tagLine == 1) {
								tagType = line.Replace(" ", "");
								switch (tagType) {
									case "int":
									case "integer":
										displayType = new DisplayType.IntSlider(
											int.Parse(lines[i + 1], NumberStyles.Any, CultureInfo.InvariantCulture),
											int.Parse(lines[i + 2], NumberStyles.Any, CultureInfo.InvariantCulture)
										);
										i += 2;
										break;
									case "float":
										displayType = new DisplayType.FloatSlider(
											float.Parse(lines[i + 1], NumberStyles.Any, CultureInfo.InvariantCulture),
											float.Parse(lines[i + 2], NumberStyles.Any, CultureInfo.InvariantCulture)
										);
										i += 2;
										break;
									case "inputstring":
										displayType = DisplayType.InputString;
										break;
									case "inputsignedfloat":
										displayType = DisplayType.InputSignedFloat;
										break;
									case "inputsignedint":
									case "inputsignedinteger":
										displayType = DisplayType.InputSignedInteger;
										break;
									case "inputunsignedfloat":
										displayType = DisplayType.InputUnsignedFloat;
										break;
									case "inputunsignedint":
									case "inputunsignedinteger":
										displayType = DisplayType.InputUnsignedInteger;
										break;
									case "none":
									case "empty":
									case "toggle":
										displayType = DisplayType.None;
										break;
								}
							}
							else if (tagLine == 2) {
								tagCreatures = [.. line.Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)];
							}
						}
					}
					if (workingTag != null && displayType != null) {
						tagData.Add(workingTag, (displayType, tagCreatures));
					}
				}
			}

			if (PathUtil.TryGetDirectory(dir, "timelines", out string? timelinesPath)) {
				foreach (string filePath in Directory.GetFiles(timelinesPath, "*.png")) {
					string timeline = Path.GetFileNameWithoutExtension(filePath);
					timelineTextures.Add(timeline.ToLowerInvariant(), Texture.Load(filePath));
					timelines.Add(timeline);
				}
			}

			if (PathUtil.TryGetDirectory(dir, "objects", out string? objectsPath)) {
				foreach (string filePath in Directory.GetFiles(objectsPath, "*.png")) {
					objectTextures.Add(Path.GetFileNameWithoutExtension(filePath).ToLowerInvariant(), Texture.Load(filePath));
				}
			}
		}

		foreach (KeyValuePair<string, (DisplayType, List<string>)> kvp in tagData) {
			List<string> tagCreatures = kvp.Value.Item2;
			if (tagCreatures.Contains("*")) {
				tagCreatures.Remove("*");
				tagCreatures.AddRange(creatures);
			}
			for (int i = tagCreatures.Count - 1; i >= 0; i--) {
				if (tagCreatures[i].StartsWith('#')) {
					tagCreatures.AddRange(tagVariables[tagCreatures[i][1..]]);
					tagCreatures.RemoveAt(i);
				}
				else if (tagCreatures[i].StartsWith('~')) {
					string c = tagCreatures[i][1..];
					tagCreatures.RemoveAt(i);
					int j = tagCreatures.IndexOf(c);
					if (j == -1) continue;
					if (j < i) i--;
					tagCreatures.RemoveAt(j);
				}
			}

			tags.Add(kvp.Key, new Tag(kvp.Key, kvp.Value.Item1, [.. tagCreatures]));
		}

		creatures.Add("UNKNOWN");
	}

	public static Texture GetCreatureTexture(string type) {
		type = type.ToLowerInvariant();

		if (type == "clear") return Clear;
		if (type == "unknown") return Unknown;

		if (creatureTextures.TryGetValue(type, out Texture? tex)) {
			return tex;
		}

		return Unknown;
	}

	public static Texture GetTagTexture(string type) {
		type = type.ToLowerInvariant();

		if (tagTextures.TryGetValue(type, out Texture? tex)) {
			return tex;
		}

		return Unknown;
	}

	public static string ParseCreature(string type) {
		if (type.Equals("NONE", StringComparison.InvariantCultureIgnoreCase)) return "";

		type = type.ToLowerInvariant();

		if (creatureParseMap.TryGetValue(type, out string? o)) {
			return o;
		}

		return type;
	}

	public static Tag GetOrCreateTag(string id) {
		if (tags.TryGetValue(id, out Tag tag))
			return tag;

		tag = new Tag(id, DisplayType.None);
		tags.Add(id, tag);
		return tag;
	}

	public static string ExportCreatureName(string type) {
		if (type.IsNullOrEmpty()) return "NONE";

		if (creatureExportNames.TryGetValue(type, out string? o)) {
			return o;
		}

		return type;
	}

	public static bool CreatureKnown(string type) {
		if (type.IsNullOrEmpty()) return true;

		return creatureTextures.ContainsKey(ParseCreature(type));
	}


	public static Texture GetTimelineTexture(string timeline) {
		timeline = timeline.ToLowerInvariant();

		if (timelineTextures.TryGetValue(timeline, out Texture? tex)) {
			return tex;
		}

		return Unknown;
	}

	public static bool HasTimeline(string timeline) {
		return timelines.Contains(timeline);
	}

	public static Texture GetObjectTexture(string type) {
		type = type.ToLowerInvariant();

		if (objectTextures.TryGetValue(type, out Texture? tex)) {
			return tex;
		}

		return Unknown;
	}

	public class DisplayType : IEquatable<DisplayType> {
		public static DisplayType None = new DisplayType("None");
		public static DisplayType InputSignedInteger = new DisplayType("InputSignedInt");
		public static DisplayType InputSignedFloat = new DisplayType("InputSignedFloat");
		public static DisplayType InputUnsignedInteger = new DisplayType("InputUnsignedInt");
		public static DisplayType InputUnsignedFloat = new DisplayType("InputUnsignedFloat");
		public static DisplayType InputString = new DisplayType("InputString");

		public readonly string id;

		private DisplayType(string id) {
			this.id = id;
		}

		public virtual void Clamp(DenCreature.Tag tag) {}

		bool IEquatable<DisplayType>.Equals(DisplayType? other) {
			return this.id == other?.id;
		}

		public class IntSlider : DisplayType {
			public readonly int min, max;

			public override void Clamp(DenCreature.Tag tag) {
				if (tag is not DenCreature.IntegerTag intTag)
					return;

				intTag.data = Math.Clamp(intTag.data, this.min, this.max);
			}

			public IntSlider(int min, int max) : base("IntSlider") {
				this.min = min;
				this.max = max;
			}
		}

		public class FloatSlider : DisplayType {
			public readonly float min, max;

			public override void Clamp(DenCreature.Tag tag) {
				if (tag is not DenCreature.FloatTag floatTag)
					return;

				floatTag.data = Mathf.Clamp(floatTag.data, this.min, this.max);
			}

			public FloatSlider(float min, float max) : base("FloatSlider") {
				this.min = min;
				this.max = max;
			}
		}
	}

	public readonly struct Tag : IEquatable<Tag> {
		public readonly string id;
		public readonly string[] supports;
		public readonly DisplayType displayType;

		public Tag(string id, DisplayType displayType, string[]? supports = null) {
			this.id = id;
			this.supports = supports ?? [];
			this.displayType = displayType;
		}

		public bool Equals(Tag other) {
			return this.id == other.id;
		}

		public static bool operator ==(Tag a, Tag b) {
			return a.Equals(b);
		}

		public static bool operator !=(Tag a, Tag b) {
			return !(a == b);
		}

		public override bool Equals(object? obj) {
			return obj is Tag other && this.Equals(other);
		}

		public override int GetHashCode() {
			return this.id?.GetHashCode() ?? 0;
		}

		public readonly bool Supports(string creature) {
			return this.supports.Length == 0 || this.supports.Contains(creature);
		}
	}
}