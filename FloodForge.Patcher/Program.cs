using System.Diagnostics;

if (args.Length < 3) return;

string sourceFolder = args[0];
string destinationFolder = args[1];
int parentPid = int.Parse(args[2]);
int version = int.Parse(args.Length >= 4 ? args[3] : "1");

string logPath = Path.Combine(destinationFolder, "patcherlog.txt");
if (File.Exists(logPath)) {
	File.Delete(logPath);
}

void LogError(Exception ex) {
	string message = $"ERROR: {ex.Message}\n{ex.StackTrace}\n";
	File.AppendAllText(logPath, message);
}

void Log(string message) {
	File.AppendAllText(logPath, message + "\n");
}

try {
	Log("Waiting for parent to close");
	Process parent = Process.GetProcessById(parentPid);
	parent.WaitForExit(5000);
}
catch {}
Log("Patching");

try {
	foreach (string dirPath in Directory.GetDirectories(sourceFolder, "*", SearchOption.AllDirectories)) {
		Directory.CreateDirectory(dirPath.Replace(sourceFolder, destinationFolder));
	}
	Log("Copied folders");

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
	Log("Copied base files");

	CopyBase(Path.Combine(sourceFolder, "docs"));
	Log("Copied docs");

	foreach (string newPath in Directory.GetFiles(Path.Combine(sourceFolder, "assets"), "*.*", SearchOption.TopDirectoryOnly)) {
		if (Path.GetFileName(newPath) == "settings.cfg") {
			string dest = newPath.Replace(sourceFolder, destinationFolder);
			MergeConfigs(newPath, dest);
			continue;
		}
		File.Copy(newPath, newPath.Replace(sourceFolder, destinationFolder), true);
	}
	Log("Copied base assets");

	CopyAll(Path.Combine(sourceFolder, "assets", "fonts"));
	Log("Copied fonts");

	CopyAll(Path.Combine(sourceFolder, "assets", "shaders"));
	Log("Copied shaders");

	CopyAll(Path.Combine(sourceFolder, "assets", "icons"));
	Log("Copied icons");

	CopyAllKeep(Path.Combine(sourceFolder, "assets", "themes"));
	Log("Copied themes");

	if (version == 1) {
		Log("Reformatting creatures");
		string sourceRoot = Path.Combine(destinationFolder, "assets", "creatures");
		string targetBase = Path.Combine(destinationFolder, "assets", "mods");
		foreach (string creatureDir in Directory.GetDirectories(sourceRoot)) {
			if (Path.GetFileName(creatureDir).Equals("room", StringComparison.InvariantCultureIgnoreCase)) continue;
			if (Path.GetFileName(creatureDir).Equals("tags", StringComparison.InvariantCultureIgnoreCase)) continue;

			string modPath = Path.Combine(targetBase, Path.GetFileName(creatureDir));
			string creaturesPath = Path.Combine(modPath, "creatures");
			CopyDirectory(creatureDir, creaturesPath);
		}
		File.Copy(Path.Combine(destinationFolder, "assets", "creatures", "mods.txt"), Path.Combine(destinationFolder, "assets", "mods.txt"), true);
		Directory.Move(sourceRoot, Path.Combine(destinationFolder, "assets", "~creatures"));
		File.AppendAllText(Path.Combine(destinationFolder, "assets", "~creatures", "README.txt"), "This is a backup folder of your creatures directory.\nIf everything looks correct in assets/mods/ then you can safely delete this folder.");
		Log("Reformatted");
	}

	string mods = Path.Combine(sourceFolder, "assets", "mods");
	foreach (string newPath in Directory.GetFiles(mods, "*.*", SearchOption.AllDirectories)) {
		File.Copy(newPath, newPath.Replace(sourceFolder, destinationFolder), true);
	}
	Log("Copied mods");

	if (version == 1) {
		Log("Reformatting timelines");
		string modsPath = Path.Combine(destinationFolder, "assets", "mods");
		string oldTimelines = Path.Combine(destinationFolder, "assets", "timelines");
		if (Directory.Exists(oldTimelines)) {
			string defaultModTimelines = Path.Combine(modsPath, "default", "timelines");
			Directory.CreateDirectory(defaultModTimelines);

			foreach (string timelineFile in Directory.GetFiles(oldTimelines, "*.*", SearchOption.TopDirectoryOnly)) {
				string fileName = Path.GetFileName(timelineFile);
				if (fileName.Equals("unknown.png", StringComparison.InvariantCultureIgnoreCase) || fileName.Equals("warning.png", StringComparison.InvariantCultureIgnoreCase)) {
					continue;
				}
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
			File.AppendAllText(Path.Combine(destinationFolder, "assets", "~timelines", "README.txt"), "This is a backup folder of your timelines directory.\nIf everything looks correct in assets/mods/ then you can safely delete this folder.");
		}
		Directory.Delete(Path.Combine(destinationFolder, "mods"));
		Directory.Delete(Path.Combine(destinationFolder, "assets", "objects"), true);
		Log("Reformatted");
	}

	Log("Launching");
	string mainExec = OperatingSystem.IsWindows() ? "FloodForge.exe" : "FloodForge";
	Process.Start(new ProcessStartInfo() {
		FileName = Path.Combine(destinationFolder, mainExec),
		Arguments = version == 1 ? $"--patcher=\"{sourceFolder}\"" : "",
		WorkingDirectory = destinationFolder
	});

	Log("Complete!");
}
catch (Exception ex) {
	LogError(ex);
	Environment.Exit(1);
}