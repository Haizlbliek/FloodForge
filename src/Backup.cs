using System.IO;

namespace FloodForge;

public static class Backup {
	private static readonly string BackupDir = "backups";

	public static void File(string filePath) {
		if (!System.IO.File.Exists(filePath))
			return;

		Directory.CreateDirectory(BackupDir);

		string stem = Path.GetFileNameWithoutExtension(filePath);
		string ext = Path.GetExtension(filePath);

		// Delete old backups
		var matchingFiles = Directory.GetFiles(BackupDir)
			.Select(f => new FileInfo(f))
			.Where(f => f.Name.StartsWith(stem + "-") && f.Name.EndsWith(ext))
			.OrderBy(f => f.LastWriteTime)
			.ToList();

		while (matchingFiles.Count >= 5) {
			matchingFiles[0].Delete();
			matchingFiles.RemoveAt(0);
		}

		// Create backup
		string timestamp = DateTime.Now.ToString("yyyyMMdd-HHmmss");
		string fileName = $"{stem}-{timestamp}{ext}";
		string destinationPath = Path.Combine(BackupDir, fileName);

		System.IO.File.Copy(filePath, destinationPath, true);
	}
}
