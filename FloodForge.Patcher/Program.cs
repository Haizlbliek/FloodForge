using System.Diagnostics;

if (args.Length < 3) return;

string sourceFolder = args[0];
string destinationFolder = args[1];
int parentPid = int.Parse(args[2]);

try {
	var parent = Process.GetProcessById(parentPid);
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

	var userSettings = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
	foreach (var line in File.ReadAllLines(destCfg)) {
		if (string.IsNullOrWhiteSpace(line) || line.TrimStart().StartsWith('#')) continue;
		var parts = line.Split('=', 2);
		if (parts.Length == 2) userSettings[parts[0].Trim()] = parts[1].Trim();
	}

	var newLines = File.ReadAllLines(sourceCfg);
	var finalLines = new List<string>();
	var handledUserKeys = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

	foreach (var line in newLines) {
		string trimmed = line.Trim();
		if (!trimmed.StartsWith('#') && trimmed.Contains('=')) {
			var parts = trimmed.Split('=', 2);
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
	foreach (var kvp in userSettings) {
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
CopyAll(Path.Combine(sourceFolder, "assets", "objects"));
CopyAll(Path.Combine(sourceFolder, "assets", "fonts"));
CopyAll(Path.Combine(sourceFolder, "assets", "shaders"));
CopyAllKeep(Path.Combine(sourceFolder, "assets", "themes"));
CopyAllKeep(Path.Combine(sourceFolder, "assets", "timelines"));
string creatures = Path.Combine(sourceFolder, "assets", "creatures");
foreach (string newPath in Directory.GetFiles(creatures, "*.*", SearchOption.AllDirectories)) {
	if (Path.GetFileName(newPath) == "mods.txt")
		continue;

	File.Copy(newPath, newPath.Replace(sourceFolder, destinationFolder), true);
}

string mainExec = OperatingSystem.IsWindows() ? "FloodForge.exe" : "FloodForge";
Process.Start(new ProcessStartInfo() {
	FileName = Path.Combine(destinationFolder, mainExec),
	WorkingDirectory = destinationFolder
});
