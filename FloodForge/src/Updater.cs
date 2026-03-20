using System.Diagnostics;
using System.IO.Compression;
using System.Net.Http;
using System.Security.Cryptography;
using System.Threading.Tasks;

namespace FloodForge;

public static class Updater {
	public static async Task Download(string url, string checksum) {
		string tempFolder = Path.Combine(Path.GetTempPath(), "FloodForge", "Extract", Path.GetRandomFileName());
		Directory.CreateDirectory(tempFolder);

		string zipFilePath = Path.Combine(tempFolder, "downloaded.zip");
		string extractPath = Path.Combine(tempFolder, "extracted_files");

		using var client = new HttpClient();
		byte[] fileBytes = await client.GetByteArrayAsync(url);

		string actualChecksum = BitConverter.ToString(SHA256.HashData(fileBytes))
											.Replace("-", "")
											.ToLowerInvariant();

		if (!string.Equals(actualChecksum, checksum, StringComparison.OrdinalIgnoreCase)) {
			throw new Exception($"Checksum mismatch! Expected: {checksum}, Actual: {actualChecksum}");
		}

		await File.WriteAllBytesAsync(zipFilePath, fileBytes);
		ZipFile.ExtractToDirectory(zipFilePath, extractPath, overwriteFiles: true);
		File.Delete(zipFilePath);

		string currentDir = AppContext.BaseDirectory;
		string patcherName = OperatingSystem.IsWindows() ? "FloodForge.Patcher.exe" : "FloodForge.Patcher";
		string patcherPath = Path.Combine(extractPath, patcherName);
		if (!File.Exists(patcherPath)) {
			patcherPath = Path.Combine(currentDir, patcherName);
		}

		if (File.Exists(patcherPath)) {
			Process.Start(new ProcessStartInfo {
				FileName = patcherPath,
				Arguments = $"\"{extractPath}\" \"{currentDir}\" \"{Process.GetCurrentProcess().Id}\"",
				UseShellExecute = true
			});

			Environment.Exit(0);
		}
		else {
			Logger.Error("Patcher not found");
		}
	}
}