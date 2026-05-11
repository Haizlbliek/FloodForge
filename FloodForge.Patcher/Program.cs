using System.Diagnostics;

if (args.Length < 3) return;

string sourceFolder = args[0];
string destinationFolder = args[1];
int parentPid = int.Parse(args[2]);
int version = int.Parse(args.Length >= 4 ? args[3] : "1");

try {
	Process parent = Process.GetProcessById(parentPid);
	parent.WaitForExit(5000);
}
catch {}

foreach (string dirPath in Directory.GetDirectories(sourceFolder, "*", SearchOption.AllDirectories)) {
	Directory.CreateDirectory(dirPath.Replace(sourceFolder, destinationFolder));
}

void CopyAll(string from) {
	if (!Directory.Exists(from)) return;

	foreach (string newPath in Directory.GetFiles(from, "*.*", SearchOption.AllDirectories)) {
		File.Copy(newPath, newPath.Replace(sourceFolder, destinationFolder), true);
	}
}

void CopyAllKeep(string from) {
	if (!Directory.Exists(from)) return;

	foreach (string newPath in Directory.GetFiles(from, "*.*", SearchOption.AllDirectories)) {
		string dest = newPath.Replace(sourceFolder, destinationFolder);
		if (!File.Exists(dest)) {
			File.Copy(newPath, dest, false);
		}
	}
}

void CopyDirectory(string sourceDir, string destDir) {
	Directory.CreateDirectory(destDir);

	foreach (string file in Directory.GetFiles(sourceDir)) {
		string destFile = Path.Combine(destDir, Path.GetFileName(file));
		File.Copy(file, destFile, true);
	}

	foreach (string subDir in Directory.GetDirectories(sourceDir)) {
		string destSubDir = Path.Combine(destDir, Path.GetFileName(subDir));
		CopyDirectory(subDir, destSubDir);
	}
}

void CopyBase(string from) {
	if (!Directory.Exists(from)) return;

	foreach (string newPath in Directory.GetFiles(from, "*.*", SearchOption.TopDirectoryOnly)) {
		File.Copy(newPath, newPath.Replace(sourceFolder, destinationFolder), true);
	}
}

void MergeConfigs(string sourceCfg, string destCfg) {
	if (!File.Exists(destCfg)) {
		File.Copy(sourceCfg, destCfg);
		return;
	}

	Dictionary<string, string> userSettings = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
	foreach (string line in File.ReadAllLines(destCfg)) {
		if (string.IsNullOrWhiteSpace(line) || line.TrimStart().StartsWith('#')) continue;
		string[] parts = line.Split('=', 2);
		if (parts.Length == 2) userSettings[parts[0].Trim()] = parts[1].Trim();
	}

	string[] newLines = File.ReadAllLines(sourceCfg);
	List<string> finalLines = new List<string>();
	HashSet<string> handledUserKeys = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

	foreach (string line in newLines) {
		string trimmed = line.Trim();
		if (!trimmed.StartsWith('#') && trimmed.Contains('=')) {
			string[] parts = trimmed.Split('=', 2);
			string key = parts[0].Trim();

			if (userSettings.TryGetValue(key, out string? val)) {
				finalLines.Add($"{key}={val}");
				handledUserKeys.Add(key);
				continue;
			}
		}
		finalLines.Add(line);
	}

	bool headerAdded = false;
	foreach (KeyValuePair<string, string> kvp in userSettings) {
		if (!handledUserKeys.Contains(kvp.Key)) {
			if (!headerAdded) {
				finalLines.Add("");
				finalLines.Add("# --- User Settings ---");
				headerAdded = true;
			}
			finalLines.Add($"{kvp.Key}={kvp.Value}");
		}
	}

	File.WriteAllLines(destCfg, finalLines);
}


foreach (string newPath in Directory.GetFiles(sourceFolder, "*.*", SearchOption.TopDirectoryOnly)) {
	if (Path.GetFileName(newPath).Contains("FloodForge.Patcher")) continue;
	File.Copy(newPath, newPath.Replace(sourceFolder, destinationFolder), true);
}

CopyBase(Path.Combine(sourceFolder, "docs"));
foreach (string newPath in Directory.GetFiles(Path.Combine(sourceFolder, "assets"), "*.*", SearchOption.TopDirectoryOnly)) {
	if (Path.GetFileName(newPath) == "settings.cfg") {
		string dest = newPath.Replace(sourceFolder, destinationFolder);
		MergeConfigs(newPath, dest);
		continue;
	}
	File.Copy(newPath, newPath.Replace(sourceFolder, destinationFolder), true);
}
CopyAll(Path.Combine(sourceFolder, "assets", "fonts"));
CopyAll(Path.Combine(sourceFolder, "assets", "shaders"));
CopyAll(Path.Combine(sourceFolder, "assets", "icons"));
CopyAllKeep(Path.Combine(sourceFolder, "assets", "themes"));
if (version == 1) {
	Directory.CreateDirectory(Path.Combine(destinationFolder, "assets", "mods"));
	string sourceRoot = Path.Combine(destinationFolder, "assets", "creatures");
	string targetBase = Path.Combine(destinationFolder, "assets", "mods");
	foreach (string creatureDir in Directory.GetDirectories(sourceRoot)) {
		if (Path.GetFileName(creatureDir).Equals("room", StringComparison.InvariantCultureIgnoreCase)) continue;
		if (Path.GetFileName(creatureDir).Equals("tags", StringComparison.InvariantCultureIgnoreCase)) continue;

		string modPath = Path.Combine(targetBase, Path.GetFileName(creatureDir));
		string creaturesPath = Path.Combine(modPath, "creatures");
		CopyDirectory(creatureDir, creaturesPath);
	}
	Directory.Move(sourceRoot, Path.Combine(destinationFolder, "assets", "~creatures"));
}
string mods = Path.Combine(sourceFolder, "assets", "mods");
foreach (string newPath in Directory.GetFiles(mods, "*.*", SearchOption.AllDirectories)) {
	File.Copy(newPath, newPath.Replace(sourceFolder, destinationFolder), true);
}
if (version == 1) {
	string modsPath = Path.Combine(destinationFolder, "assets", "mods");
	string oldTimelines = Path.Combine(destinationFolder, "assets", "timelines");
	if (Directory.Exists(oldTimelines)) {
		string defaultModTimelines = Path.Combine(modsPath, "default", "timelines");
		Directory.CreateDirectory(defaultModTimelines);

		foreach (string timelineFile in Directory.GetFiles(oldTimelines, "*.*", SearchOption.TopDirectoryOnly)) {
			string fileName = Path.GetFileName(timelineFile);
			bool existsInAnyMod = false;

			foreach (string modDir in Directory.GetDirectories(modsPath)) {
				if (File.Exists(Path.Combine(modDir, "timelines", fileName))) {
					existsInAnyMod = true;
					break;
				}
			}

			if (!existsInAnyMod) {
				File.Copy(timelineFile, Path.Combine(defaultModTimelines, fileName), true);
			}
		}
		Directory.Move(oldTimelines, Path.Combine(destinationFolder, "assets", "~timelines"));
	}
}

string mainExec = OperatingSystem.IsWindows() ? "FloodForge.exe" : "FloodForge";
Process.Start(new ProcessStartInfo() {
	FileName = Path.Combine(destinationFolder, mainExec),
	Arguments = version == 1 ? $"--patcher=\"{sourceFolder}\"" : "",
	WorkingDirectory = destinationFolder
});