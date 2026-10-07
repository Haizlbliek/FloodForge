using System.Globalization;
using FloodForge.SettingTypes;

namespace FloodForge;

public static class Settings {
	/// <summary>
	/// The config number that the current version of FloodForge expects. To avoid FloodForge upgrading the settings.cfg file on a fresh build, update the repository's settings.cfg 'configVersion' value.
	/// </summary>
	private const int programConfigVersion = 1;
	private const string settingsPath = "assets/settings.cfg";

	public static Dictionary<string, Setting> settings = [];

	public static Setting<float> CameraPanSpeed = Setting.Of("CameraPanSpeed", 0.4f);
	public static Setting<float> CameraZoomSpeed = Setting.Of("CameraZoomSpeed", 0.4f);
	public static Setting<float> PopupScrollSpeed = Setting.Of("PopupScrollSpeed", 0.4f);
	public static Setting<STConnectionType> ConnectionType = Setting.Of("ConnectionType", STConnectionType.Bezier);
	public static Setting<STConnectionPoint> ConnectionPoint = Setting.Of("ConnectionPoint", STConnectionPoint.Entrance);
	public static Setting<bool> OriginalControls = Setting.Of("OriginalControls", false);
	public static Setting<float> WorldIconScale = Setting.Of("WorldIconScale", 1f).Override(value => value.Equals("camera", StringComparison.InvariantCultureIgnoreCase) ? (true, -1) : (false, default));
	public static Setting<string> DefaultFilePath = Setting.Of("DefaultFilePath", "");
	public static Setting<bool> KeepFilesystemPath = Setting.Of("KeepFilesystemPath", false);
	public static Setting<bool> WarnMissingImages = Setting.Of("WarnMissingImages", false);
	public static Setting<bool> HideTutorial = Setting.Of("HideTutorial", false);
	public static Setting<bool> HideTutorialOnLoadWorld = Setting.Of("HideTutorialOnLoadWorld", false);
	public static Setting<bool> UpdateRegionFiles = Setting.Of("UpdateRegionFiles", true);
	public static Setting<bool> UpdateRoomImagesOnRender = Setting.Of("UpdateRoomImagesOnRender", false);
	public static Setting<Color> NoSubregionColor = Setting.Of("NoSubregionColor", Color.White);
	public static Setting<float> RoomTintStrength = Setting.Of("RoomTintStrength", 0.5f);
	public static Setting<bool> DropdownOnHover = Setting.Of("DropdownOnHover", false);
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
	public static Setting<bool> ExportPsdFiles = Setting.Of("ExportPsdFiles", false);

	public static Setting<bool> DEBUGVisibleOutputPadding = Setting.Of("DebugVisibleOutputPadding", false);
	public static Setting<bool> DEBUGVisiblePopupVisuals = Setting.Of("DebugVisiblePopupVisuals", false);
	public static Setting<bool> DEBUGVisibleConnectionBounds = Setting.Of("DebugVisibleConnectionBounds", false);
	public static Setting<bool> DEBUGVisibleShortcutEntranceData = Setting.Of("DebugVisibleShortcutEntranceData", false);
	public static Setting<bool> DEBUGRoomWireframe = Setting.Of("DebugRoomWireframe", false);
	public static Setting<bool> DEBUGLogInvalidSlopes = Setting.Of("DebugLogInvalidSlopes", false);
	public static Setting<bool> DEBUGVerboseExportLog = Setting.Of("DebugVerboseExportLog", false);


	private static int loadedConfigVersion = -1; // -1 by default: is set if a cfgVersion key is present in the loaded settings.cfg file

	public static void Initialize() {
		string[] lines = File.ReadAllLines(settingsPath);

		foreach (string l in lines) {
			string line = l.Trim();
			if (line == "" || line.StartsWith('#')) continue;

			string key = line[..line.IndexOf('=')].Trim();
			string value = line[(line.IndexOf('=') + 1)..].Trim();

			if (key.Equals("cfgVersion", StringComparison.InvariantCultureIgnoreCase)) {
				if (int.TryParse(value, out int version)) {
					loadedConfigVersion = version;
					Logger.Note($"cfgVersion: {loadedConfigVersion}");
				}
				else {
					Logger.Warn($"failed to parse cfgVersion value \"{value}\" to int");
				}
				continue;
			}
		}

		bool triedToUpdate = false;
		if (loadedConfigVersion < programConfigVersion) {
			Logger.Info($"Outdated config version detected! ({loadedConfigVersion} -> {programConfigVersion})");
			UpgradeConfigFile([.. lines], loadedConfigVersion, programConfigVersion); // TODO - ask user before doing this
			triedToUpdate = true;
		}
		if (loadedConfigVersion > programConfigVersion) {
			Logger.Warn($"Config version is newer than expected! ({loadedConfigVersion} -> {programConfigVersion})");
		}

		lines = File.ReadAllLines(settingsPath);

		foreach (string l in lines) {
			string line = l.Trim();
			if (line == "" || line.StartsWith('#')) continue;

			string key = line[..line.IndexOf('=')].Trim();
			string value = line[(line.IndexOf('=') + 1)..].Trim();
			if (key == "Theme") {
				Themes.LoadFromSetting(value);
				continue;
			}

			if (triedToUpdate && key.Equals("cfgVersion", StringComparison.InvariantCultureIgnoreCase)) {
				if (int.TryParse(value, out int version)) {
					loadedConfigVersion = version;
					Logger.Note($"cfgVersion: {loadedConfigVersion}");
				}
				else {
					Logger.Warn($"failed to parse cfgVersion value \"{value}\" to int");
				}
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

	private static void UpgradeConfigFile(List<string> settingsFile, int upgradeFromVersion, int upgradeToVersion) {
		bool success = true;
		string message = ""; // used to report any issues encountered during any upgrade step.

		//THIS IS WHERE CHECKS WOULD GO; make sure they are in order of cfgVersion (so version 2's changes are applied before version 4's changes)
		//example of a check: if cfgVersion 3 changes the name of the setting "DoFunnyThings" to "DoSillyThings", the check might look like:
		//	if (upgradeFromVersion < 3) {
		//		int DoFunnyThingsIndex = settingsFile.FindIndex(s => s.StartsWith("DoFunnyThings="));
		//		settingsFile[DoFunnyThingsIndex] = $"DoSillyThings={settingsFile[DoFunnyThingsIndex].Split('=')[^1]}";
		//	}
		//Granted, this system may change. For example, it's harder to update the description that accompanies a setting in the case where the patcher wasn't used to update.

		if (upgradeFromVersion < 1) {
			int updateWorldFilesIndex = settingsFile.FindIndex(s => s.StartsWith("UpdateWorldFiles"));
			if (updateWorldFilesIndex != -1) {
				string value = settingsFile[updateWorldFilesIndex].Split('=')[^1];

				if (settingsFile[updateWorldFilesIndex - 1].StartsWith("# ") && settingsFile[updateWorldFilesIndex - 2].StartsWith("# ")) {
					settingsFile[updateWorldFilesIndex - 2] = "# If true, exporting will modify the existing files that exist the region's folder (`world_xx.txt`, `map_xx.txt/png`, etc.)";
					settingsFile[updateWorldFilesIndex - 1] = "# If false, exported regions are stored in `FloodForge/worlds` and need to be manually copied into their respective directories";
				}
				settingsFile[updateWorldFilesIndex] = $"UpdateRegionFiles={value}";
			}
		}

		if (success) {
			try {
				Logger.Info("Incrementing cfgVersion");
				int versionIndex = settingsFile.FindIndex(s => s.StartsWith("cfgVersion="));
				if (versionIndex != -1)
					settingsFile[versionIndex] = $"cfgVersion={upgradeToVersion}";
				else {
					settingsFile.Add($"");
					settingsFile.Add($"# This config file's version. Do not modify!");
					settingsFile.Add($"cfgVersion={upgradeToVersion}");
				}
				File.WriteAllText(settingsPath, string.Join('\n', [.. settingsFile]));
			}
			catch (Exception e) {
				message = e.ToString();
				success = false;
			}
		}

		if (success) {
			Logger.Info("Modifications successful.");
		}
		else {
			Logger.Error($"Modifications failed: {message}");
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