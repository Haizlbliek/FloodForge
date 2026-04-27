using System.Globalization;
using FloodForge.SettingTypes;

namespace FloodForge;

public static class Settings {
	public static Dictionary<string, Setting> settings = [];

	public static Setting<float> CameraPanSpeed = Setting.Of("CameraPanSpeed", 0.4f);
	public static Setting<float> CameraZoomSpeed = Setting.Of("CameraZoomSpeed", 0.4f);
	public static Setting<float> PopupScrollSpeed = Setting.Of("PopupScrollSpeed", 0.4f);
	public static Setting<STConnectionType> ConnectionType = Setting.Of("ConnectionType", STConnectionType.Bezier);
	public static Setting<STConnectionPoint> ConnectionPoint = Setting.Of("ConnectionPoint", STConnectionPoint.Entrance);
	public static Setting<float> WorldIconScale = Setting.Of("WorldIconScale", 1f).Override(value => value.Equals("camera", StringComparison.InvariantCultureIgnoreCase) ? (true, -1) : (false, default));
	public static Setting<string> DefaultFilePath = Setting.Of("DefaultFilePath", "");
	public static Setting<bool> OriginalControls = Setting.Of("OriginalControls", false);
	public static Setting<bool> WarnMissingImages = Setting.Of("WarnMissingImages", false);
	public static Setting<bool> HideTutorial = Setting.Of("HideTutorial", false);
	public static Setting<bool> KeepFilesystemPath = Setting.Of("KeepFilesystemPath", false);
	public static Setting<bool> UpdateWorldFiles = Setting.Of("UpdateWorldFiles", true);
	public static Setting<Color> NoSubregionColor = Setting.Of("NoSubregionColor", Color.White);
	public static Setting<float> RoomTintStrength = Setting.Of("RoomTintStrength", 0.5f);
	public static Setting<STDisabledButtonsMode> DisabledButtonsMode = Setting.Of("DisabledButtonsMode", STDisabledButtonsMode.Grey);
	public static Setting<STForceExportCasing> ForceExportCasing = Setting.Of("ForceExportCasing", STForceExportCasing.None);
	public static Setting<STDropletGridVisibility> DropletGridVisibility = Setting.Of("DropletGridVisibility", STDropletGridVisibility.Air);
	public static Setting<bool> DropletKeepRelativePosition = Setting.Of("DropletKeepRelativePosition", true);
	public static Setting<float> ConnectionOpacity = Setting.Of("ConnectionOpacity", 1f);
	public static SubregionColorsSetting SubregionColors = new SubregionColorsSetting("SubregionColors", [ Color.Red, Color.Green, Color.Blue, Color.Yellow, Color.Cyan, Color.Magenta, new Color(1f, 0.5f, 0f), new Color(0.5f, 0.5f, 0.5f), new Color(0.5f, 0f, 1f), new Color(1f, 0.5f, 1f) ]);
	public static Setting<bool> DisableAprilFoolsUpdates = Setting.Of("DisableAprilFoolsUpdates", false);
	public static Setting<bool> DiscordRichPresence = Setting.Of("DiscordRichPresence", true);
	public static Setting<bool> RoundedUI = Setting.Of("RoundedUI", false);
	public static Setting<bool> DisableUpdater = Setting.Of("DisableUpdater", false);
	public static Setting<string> RainedPath = Setting.Of("RainedPath", "");

	public static Setting<bool> DEBUGVisibleOutputPadding = Setting.Of("DebugVisibleOutputPadding", false);
	public static Setting<bool> DEBUGVisiblePopupVisuals = Setting.Of("DebugVisiblePopupVisuals", false);
	public static Setting<bool> DEBUGVisibleConnectionBounds = Setting.Of("DebugVisibleConnectionBounds", false);
	public static Setting<bool> DEBUGVisibleShortcutEntranceData = Setting.Of("DebugVisibleShortcutEntranceData", false);
	public static Setting<bool> DEBUGRoomWireframe = Setting.Of("DebugRoomWireframe", false);
	public static Setting<bool> DEBUGLogInvalidSlopes = Setting.Of("DebugLogInvalidSlopes", false);


	public static void Initialize() {
		string[] lines = File.ReadAllLines("assets/settings.cfg");

		foreach (string l in lines) {
			string line = l.Trim();
			if (line == "" || line.StartsWith('#')) continue;

			string key = line[..line.IndexOf('=')].Trim();
			string value = line[(line.IndexOf('=') + 1)..].Trim();
			if (key == "Theme") {
				Themes.LoadFromSetting(value);
				continue;
			}

			if (settings.TryGetValue(key, out Setting? setting)) {
				setting.Set(value);
			}
			else {
				Logger.Warn($"No setting '{key}'");
			}
		}
	}
	public class STDisabledButtonsMode : SettingType<STDisabledButtonsMode> {
		public static readonly STDisabledButtonsMode None = STDisabledButtonsMode.Of("None");
		public static readonly STDisabledButtonsMode Grey = STDisabledButtonsMode.Of("Grey");
		public static readonly STDisabledButtonsMode Hide = STDisabledButtonsMode.Of("Hide");
	}

	public class STConnectionType : SettingType<STConnectionType> {
		public static readonly STConnectionType Bezier = STConnectionType.Of("Bezier");
		public static readonly STConnectionType Linear = STConnectionType.Of("Linear");
	}

	public class STConnectionPoint : SettingType<STConnectionPoint> {
		public static readonly STConnectionPoint Entrance = STConnectionPoint.Of("Entrance");
		public static readonly STConnectionPoint Exit = STConnectionPoint.Of("Exit");
	}

	public class STForceExportCasing : SettingType<STForceExportCasing> {
		public static readonly STForceExportCasing None = STForceExportCasing.Of("None");
		public static readonly STForceExportCasing Lower = STForceExportCasing.Of("Lower");
		public static readonly STForceExportCasing Upper = STForceExportCasing.Of("Upper");
		public static readonly STForceExportCasing MatchAcronym = STForceExportCasing.Of("MatchAcronym");
	}

	public class STDropletGridVisibility : SettingType<STDropletGridVisibility> {
		public static readonly STDropletGridVisibility None = STDropletGridVisibility.Of("None");
		public static readonly STDropletGridVisibility All = STDropletGridVisibility.Of("All");
		public static readonly STDropletGridVisibility Air = STDropletGridVisibility.Of("Air");
	}


	public abstract class Setting {
		public readonly string id;

		public abstract void Set(string value);

		public Setting(string id) {
			this.id = id;
			Settings.settings.Add(this.id, this);
		}

		public static Setting<T> Of<T>(string id, T defaultValue) where T : IParsable<T> {
			return new Setting<T>(id, defaultValue);
		}
	}

	public class Setting<T> : Setting where T : IParsable<T> {
		private readonly List<Func<string, (bool, T)>> overrides = [];
		public T value;

		public static implicit operator T(Setting<T> setting) {
			return setting.value;
		}

		public override string ToString() {
			return this.value?.ToString() ?? "";
		}

		public override void Set(string stringValue) {
			foreach (Func<string, (bool, T)> func in this.overrides) {
				(bool, T) t = func(stringValue);
				if (t.Item1) {
					this.value = t.Item2;
					return;
				}
			}

			this.value = T.Parse(stringValue, CultureInfo.InvariantCulture);
		}

		public Setting<T> Override(Func<string, (bool, T)> func) {
			this.overrides.Add(func);
			return this;
		}

		public Setting(string id, T value) : base(id) {
			this.value = value;
		}
	}

	public class SubregionColorsSetting : Setting {
		public Color[] Value { get; private set; }

		public SubregionColorsSetting(string id, Color[] colors) : base(id) {
			this.Value = colors;
		}

		public override void Set(string value) {
			string[] values = value.Split(',');
			this.Value = [.. values.Select(value => Color.Parse(value.Trim(), null))];
		}
	}
}