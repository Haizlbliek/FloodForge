using System.Diagnostics.CodeAnalysis;

public static class PathUtil {
	public static string Combine(string a, string b) {
		return Path.GetFullPath(Path.Combine(a, b));
	}

	public static string Parent(string path) {
		return Path.GetFullPath(Path.Combine(path, ".."));
	}

	public static string? FindFile(string parent, string fileName) {
		string[] files = Directory.GetFiles(parent, fileName, new EnumerationOptions() { MatchCasing = MatchCasing.CaseInsensitive, RecurseSubdirectories = false });
		if (files.Length == 0) return null;
		return files[0];
	}

	public static string FindOrAssumeFile(string parent, string fileName) {
		return FindFile(parent, fileName) ?? Combine(parent, fileName);
	}

	public static bool TryGetFile(string parent, string fileName, [NotNullWhen(true)] out string? path) {
		path = FindFile(parent, fileName);
		return path != null;
	}

	public static string? FindDirectory(string parent, string directoryName) {
		string[] dirs = Directory.GetDirectories(parent, directoryName, new EnumerationOptions() { MatchCasing = MatchCasing.CaseInsensitive, RecurseSubdirectories = false });
		if (dirs.Length == 0) return null;
		return dirs[0];
	}

	public static string FindOrAssumeDirectory(string parent, string directoryName) {
		return FindDirectory(parent, directoryName) ?? Combine(parent, directoryName);
	}

	public static bool TryGetDirectory(string parent, string directoryName, [NotNullWhen(true)] out string? path) {
		path = FindDirectory(parent, directoryName);
		return path != null;
	}
}