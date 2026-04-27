using System.Diagnostics;
using System.IO.Compression;
using System.Net.Http;
using System.Security.Cryptography;
using System.Threading.Tasks;

namespace FloodForge;

public static class Updater {
	private static readonly HttpClient _client = new HttpClient {
		Timeout = TimeSpan.FromMinutes(3)
	};

	public static async Task Download(string url, string checksum) {
		if (!_client.DefaultRequestHeaders.UserAgent.Any()) {
			_client.DefaultRequestHeaders.UserAgent.ParseAdd("FloodForge-Updater/1.0");
		}

		string tempFolder = Path.Combine(Path.GetTempPath(), "FloodForge", "Extract", Path.GetRandomFileName());
		Directory.CreateDirectory(tempFolder);

		string zipFilePath = Path.Combine(tempFolder, "downloaded.zip");
		string extractPath = Path.Combine(tempFolder, "extracted_files");

		using (HttpResponseMessage response = await _client.GetAsync(url, HttpCompletionOption.ResponseHeadersRead)) {
			response.EnsureSuccessStatusCode();

			using Stream contentStream = await response.Content.ReadAsStreamAsync();
			using FileStream fileStream = new FileStream(zipFilePath, FileMode.Create, FileAccess.Write, FileShare.None, 8192, true);
			await contentStream.CopyToAsync(fileStream);
		}

		string actualChecksum;
		using (FileStream stream = File.OpenRead(zipFilePath)) {
			byte[] hashBytes = await SHA256.HashDataAsync(stream);
			actualChecksum = BitConverter.ToString(hashBytes).Replace("-", "").ToLowerInvariant();
		}

		if (!string.Equals(actualChecksum, checksum, StringComparison.OrdinalIgnoreCase)) {
			throw new Exception($"Checksum mismatch! Expected: {checksum}, Actual: {actualChecksum}");
		}

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

			Main.Cleanup();
			Environment.Exit(0);
		}
		else {
			Logger.Error("Patcher not found");
		}
	}
}