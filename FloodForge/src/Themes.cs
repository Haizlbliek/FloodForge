using System.Runtime.CompilerServices;

namespace FloodForge;

public static class Themes {
	private static readonly Dictionary<string, int> ids = [];
	private static Color[] colors = new Color[16];
	private static int length = 0;
	private static string[] activeThemes = null!;

	public static readonly ThemeColor Background = Register("Background", new Color(0.3f, 0.3f, 0.3f));
	public static readonly ThemeColor Grid = Register("Grid", new Color(0.75f, 0.75f, 0.75f));
	public static readonly ThemeColor Border = Register("Border", new Color(0f, 1f, 1f));
	public static readonly ThemeColor BorderHighlight = Register("BorderHighlight", new Color(0f, 0f, 0f));
	public static readonly ThemeColor Popup = Register("Popup", new Color(0.2f, 0.2f, 0.2f));
	public static readonly ThemeColor PopupHeader = Register("PopupHeader", new Color(0.2f, 0.2f, 0.2f));
	public static readonly ThemeColor Button = Register("Button", new Color(0.2f, 0.2f, 0.2f));
	public static readonly ThemeColor ButtonDisabled = Register("ButtonDisabled", new Color(1f, 1f, 1f));
	public static readonly ThemeColor Text = Register("Text", new Color(0.5f, 0.5f, 0.5f));
	public static readonly ThemeColor TextDisabled = Register("TextDisabled", new Color(0f, 1f, 1f));
	public static readonly ThemeColor TextHighlight = Register("TextHighlight", new Color(0.3f, 0.3f, 0.3f));
	public static readonly ThemeColor SelectionBorder = Register("SelectionBorder", new Color(0.3f, 0.3f, 0.3f));
	public static readonly ThemeColor RoomBorder = Register("RoomBorder", new Color(0.6f, 0.6f, 0.6f));
	public static readonly ThemeColor RoomBorderHighlight = Register("RoomBorderHighlight", new Color(0.00f, 0.75f, 0.00f));
	public static readonly ThemeColor RoomAir = Register("RoomAir", new Color(1f, 1f, 1f));
	public static readonly ThemeColor RoomSolid = Register("RoomSolid", new Color(0f, 0f, 0f));
	public static readonly ThemeColor RoomLayer2Solid = Register("RoomLayer2Solid", new Color(0.75f, 0.75f, 0.75f));
	public static readonly ThemeColor RoomPole = Register("RoomPole", new Color(0f, 0f, 0f));
	public static readonly ThemeColor RoomPlatform = Register("RoomPlatform", new Color(0f, 0f, 0f));
	public static readonly ThemeColor RoomWater = Register("RoomWater", new Color(0f, 0f, 0.5f, 0.5f));
	public static readonly ThemeColor RoomShortcutEntrance = Register("RoomShortcutEntrance", new Color(0f, 1f, 1f));
	public static readonly ThemeColor RoomShortcutDot = Register("RoomShortcutDot", new Color(1f, 1f, 1f));
	public static readonly ThemeColor RoomShortcutRoom = Register("RoomShortcutRoom", new Color(1f, 0f, 1f));
	public static readonly ThemeColor RoomShortcutDen = Register("RoomShortcutDen", new Color(0f, 1f, 0f));
	public static readonly ThemeColor RoomConnection = Register("RoomConnection", new Color(1f, 1f, 0f));
	public static readonly ThemeColor RoomConnectionHover = Register("RoomConnectionHover", new Color(0f, 1f, 1f));
	public static readonly ThemeColor Layer0Color = Register("Layer0Color", new Color(1f, 0f, 0f));
	public static readonly ThemeColor Layer1Color = Register("Layer1Color", new Color(1f, 1f, 1f));
	public static readonly ThemeColor Layer2Color = Register("Layer2Color", new Color(0f, 1f, 0f));

	private static ThemeColor Register(string key, Color def) {
		int id = length++;
		if (id >= colors.Length) {
			Array.Resize(ref colors, colors.Length * 2);
		}

		colors[id] = def;
		ids.Add(key, id);
		return new ThemeColor(id);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static Color GetInternal(int idx) => Unsafe.Add(ref MemoryMarshal.GetArrayDataReference(colors), (nint)idx);

	public readonly struct ThemeColor(int id) {
		private readonly int _id = id;

		public static implicit operator Color(ThemeColor theme) => GetInternal(theme._id);
	}

	public static void Load(string theme) {
		string[] lines = File.ReadAllLines($"assets/themes/{theme}/theme.cfg");

		foreach (string l in lines) {
			string line = l.Trim();
			if (line == "" || line.StartsWith('#')) continue;

			string key = line[..line.IndexOf('=')].Trim();
			string value = line[(line.IndexOf('=') + 1)..].Trim();

			if (ids.TryGetValue(key, out int idx)) {
				colors[idx] = Color.Parse(value, null);
			} else {
				Logger.Warn($"No theme color: '{key}'");
			}
		}
	}

	public static void LoadFromSetting(string value) {
		activeThemes = value.Split(',');
		foreach (string theme in activeThemes) {
			Load(theme.Trim());
		}
	}

	public static string GetPath(string fileName) {
		for (int i = activeThemes.Length - 1; i >= 0; i--) {
			string path = PathUtil.Combine("assets/themes/" + activeThemes[i], fileName);
			if (File.Exists(path)) return path;
		}

		return PathUtil.Combine("assets/", fileName);
	}
}