using System.Diagnostics;
using System.Net.Http;
using System.Text.Json.Nodes;
using System.Threading.Tasks;
using FloodForge.Popups;
using Stride.Core.Extensions;

namespace FloodForge.World;

public class SplashArtPopup : Popup {
	protected Texture splashArt;
	protected Texture uiIcons;
	protected AppVersion version;
	protected readonly List<IconButton> buttons = [];
	protected DateTime? nightlyBuildDate;
	protected UpdateStatus updateStatus = UpdateStatus.Searching;
	protected string? updatePath;
	protected string? updateChecksum;

	protected enum UpdateStatus {
		Searching,
		Failed,
		Available,
		Unavailable,
	}

	public SplashArtPopup() {
		this.bounds = new Rect(-1f, -1f, 1f, 1f);
		this.splashArt = Texture.Load(Main.AprilFools ? "assets/splash-corrupted.png" : "assets/splash.png");
		this.uiIcons = Texture.Load("assets/uiIcons.png");

		this.version = new AppVersion(File.ReadAllText("assets/version.txt"));
		if (File.Exists("assets/nightly.txt") && DateTime.TryParse(File.ReadAllText("assets/nightly.txt").Trim(), out DateTime date)) {
			this.nightlyBuildDate = date;
		}
		if (!Settings.DisableUpdater) {
			this.CheckForUpdates();
		}

		this.buttons.Add(new IconButton("Discord Server", 0f, 0f, 0.25f, 0.25f, () => {
			Process.Start(new ProcessStartInfo() { FileName = "https://discord.gg/k5BExadp4x", UseShellExecute = true });
			return false;
		}));

		if (Main.Anniversary) {
			this.buttons.Add(new IconButton("Anniversary Event", 0.25f, 0f, 0.5f, 0.25f, () => {
				PopupManager.Add(new MarkdownPopup("docs/anniversary.md"));
				return true;
			}));
		}

		this.buttons.Add(new IconButton("Support FloodForge on Ko-Fi!", 0f, 0.25f, 0.25f, 0.5f, () => {
			Process.Start(new ProcessStartInfo() { FileName = "https://ko-fi.com/haizlbliek", UseShellExecute = true });
			return false;
		}));
	}

	private class FailedException : Exception {}

	private static readonly HttpClient client = new HttpClient(new HttpClientHandler()) {
		DefaultRequestHeaders = { { "User-Agent", "FloodForge-Updater" } }
	};

	private async void CheckForUpdates() {
		try {
			string currentUrl = this.nightlyBuildDate == null
				? "https://api.github.com/repos/Haizlbliek/FloodForge/releases/latest"
				: "https://api.github.com/repos/Haizlbliek/FloodForge/releases/tags/nightly";

			Stream currentResponse = await client.GetStreamAsync(currentUrl);
			JsonNode? currentNode = await JsonNode.ParseAsync(currentResponse) ?? throw new FailedException();

			bool currentAvailable = false;
			if (this.nightlyBuildDate == null) {
				string? value = (string?)currentNode["tag_name"] ?? throw new FailedException();
				AppVersion latest = new AppVersion(value);
				currentAvailable = this.version < latest;
			}
			else {
				string? body = currentNode["body"]?.ToString().Replace("BUILD_DATE: ", "").Trim();
				if (body == null || !DateTime.TryParse(body, out DateTime date))
					throw new FailedException();

				currentAvailable = this.nightlyBuildDate < date;
			}

			this.updateStatus = currentAvailable ? UpdateStatus.Available : UpdateStatus.Unavailable;

			string switchText = this.nightlyBuildDate == null ? "Switch to Nightly" : "Switch to Release";
			this.buttons.Add(new IconButton(switchText, 0.25f, 0.25f, 0.5f, 0.5f, () => {
				PopupManager.Add(new ConfirmPopup($"Switch to the latest {(this.nightlyBuildDate == null ? "Nightly" : "Release")} build?").Okay(async () => {
					PopupManager.Add(new InfoPopup("Fetching alternate branch info..."));
					await this.FetchAndDownloadTrack(this.nightlyBuildDate == null);
				}));
				return true;
			}));

			if (this.updateStatus == UpdateStatus.Available) {
				(string? downloadUrl, string? checksum) = this.ParseAssetData(currentNode);
				if (downloadUrl == null || checksum == null)
					throw new FailedException();

				this.updatePath = downloadUrl;
				this.updateChecksum = checksum;

				this.buttons.Add(new IconButton("Update Available!", 0.5f, 0f, 0.75f, 0.25f, () => {
					PopupManager.Add(new ConfirmPopup($"Update to latest {(this.nightlyBuildDate == null ? "version" : "nightly")}?").Okay(async () => {
						PopupManager.Add(new InfoPopup("Downloading, please wait."));
						Logger.Note($"Downloading {this.updatePath} {this.updateChecksum}");
						await Updater.Download(this.updatePath, this.updateChecksum!);
						Logger.Note($"Downloaded");
					}));
					return true;
				}));
			}
		}
		catch (Exception ex) when (ex is HttpRequestException or FailedException) {
			this.HandleUpdateFailure(ex is not FailedException ? ex : null);
		}
	}

	private async Task FetchAndDownloadTrack(bool targetIsNightly) {
		try {
			string targetUrl = targetIsNightly
				? "https://api.github.com/repos/Haizlbliek/FloodForge/releases/tags/nightly"
				: "https://api.github.com/repos/Haizlbliek/FloodForge/releases/latest";

			Stream response = await client.GetStreamAsync(targetUrl);
			JsonNode? node = await JsonNode.ParseAsync(response) ?? throw new FailedException();

			(string? downloadUrl, string? checksum) = this.ParseAssetData(node);
			if (downloadUrl == null || checksum == null)
				throw new FailedException();

			if (!targetIsNightly) {
				string nightlyFilePath = Path.Combine("assets", "nightly.txt");
				if (File.Exists(nightlyFilePath)) {
					File.Delete(nightlyFilePath);
				}
			}

			PopupManager.Add(new InfoPopup($"Downloading target branch, please wait..."));
			Logger.Note($"Switching track: Downloading {downloadUrl} {checksum}");
			await Updater.Download(downloadUrl, checksum);
			Logger.Note($"Downloaded successfully");
		}
		catch (Exception ex) {
			Logger.Warn("Failed to switch tracks");
			Logger.Info(ex);
			PopupManager.Add(new InfoPopup("Failed to download the alternative build."));
		}
	}

	private void HandleUpdateFailure(Exception? ex) {
		Logger.Warn("Failed to fetch update information");
		if (ex != null) Logger.Info(ex);

		this.updateStatus = UpdateStatus.Failed;

		IconButton b = null!;
		b = new IconButton("Failed to fetch Update", 0.75f, 0f, 1f, 0.25f, () => {
			this.updateStatus = UpdateStatus.Searching;
			this.CheckForUpdates();
			return false;
		});
		this.buttons.Add(b);
	}

	private (string? downloadUrl, string? checksum) ParseAssetData(JsonNode node) {
		string osPart = RuntimeInformation.IsOSPlatform(OSPlatform.Windows) ? "Windows" : "Linux";
		string archPart = RuntimeInformation.ProcessArchitecture == Architecture.Arm64 ? "-arm64" : "";
		string targetFileName = $"FloodForge-{osPart}{archPart}.zip";

		JsonArray? assets = node["assets"]?.AsArray();
		if (assets != null) {
			foreach (JsonNode? asset in assets) {
				if (asset?["name"]?.ToString().Equals(targetFileName, StringComparison.InvariantCultureIgnoreCase) ?? false) {
					string? downloadUrl = asset["browser_download_url"]?.ToString();
					string? digest = asset["digest"]?.ToString();
					string? checksum = digest?.Replace("sha256:", "");
					return (downloadUrl, checksum);
				}
			}
		}
		Logger.Warn($"Could not find a release asset for {targetFileName}");
		return (null, null);
	}

	public override void Close() {
		base.Close();
		this.splashArt.Dispose();
		this.uiIcons.Dispose();
	}

	public override Rect InteractBounds() {
		return new Rect(-100f, -100f, 100f, 100f);
	}

	public override void Draw() {
		Immediate.Color(0f, 0f, 0f);
		UI.ButtonFillRect(-0.9f, -0.65f, 0.9f, 0.65f);

		Program.gl.Enable(EnableCap.Blend);
		Immediate.UseTexture(this.splashArt);
		Immediate.Color(0.75f, 0.75f, 0.75f);
		Immediate.Begin(Immediate.PrimitiveType.QUADS);
		Immediate.TexCoord(0f, 1f); Immediate.Vertex(-0.89f, -0.24f);
		Immediate.TexCoord(1f, 1f); Immediate.Vertex(0.89f, -0.24f);
		Immediate.TexCoord(1f, 0f); Immediate.Vertex(0.89f, 0.64f);
		Immediate.TexCoord(0f, 0f); Immediate.Vertex(-0.89f, 0.64f);
		Immediate.End();
		Program.gl.Disable(EnableCap.Blend);

		Immediate.Color(1f, 1f, 1f);
		UI.rodondo.Write(Main.AprilFools ? "FlodoFroge" : "FloodForge", 0f, 0.3f, 0.2f, Font.Align.MiddleCenter);
		UI.font.Write(Main.AprilFools ? "Wordle Editor" : "World Editor", 0f, 0.1f, 0.1f, Font.Align.MiddleCenter);
		UI.font.Write(this.version.ToString(), -0.88f, 0.63f, 0.04f);

		Immediate.Color(0.8f, 0.8f, 0.8f);
		UI.font.Write("Recent worlds:", -0.88f, -0.28f, 0.03f, Font.Align.MiddleLeft);

		for (int i = 0; i < 8; i++) {
			if (i >= RecentFiles.recents.Count)
				break;

			string recent = RecentFiles.recentNames[i];
			string acronym = "";
			if (recent.IsNullOrEmpty()) {
				recent = Path.GetFileNameWithoutExtension(RecentFiles.recents[i]);
			}
			else {
				acronym = Path.GetFileNameWithoutExtension(RecentFiles.recents[i])["world_".Length..];
			}

			float y = -0.33f - i * 0.04f;
			Rect rect = new Rect(-0.89f, y - 0.02f, -0.4f, y + 0.015f);

			if (rect.Inside(Mouse.X, Mouse.Y)) {
				Immediate.Color(0.25f, 0.25f, 0.25f);
				UI.FillRect(rect);

				if (Mouse.JustLeft) {
					this.Close();
					if (!WorldParser.ImportWorldFile(RecentFiles.recents[i]))
						PopupManager.Add(new InfoPopup("Importing world failed!\nView log.txt for more info."));
					return;
				}
			}

			Immediate.Color(1f, 1f, 1f);
			UI.font.Write(recent + (acronym != "" ? $" ({acronym})" : ""), -0.88f, y, 0.03f, Font.Align.MiddleLeft);
		}

		Immediate.Color(1f, 1f, 1f);
		UI.ButtonStrokeRect(-0.9f, -0.65f, 0.9f, 0.65f);
		UI.Line(-0.9f, -0.25f, 0.9f, -0.25f);

		const float rowHeight = 0.06f;
		const float startY = -0.31f;

		for (int i = 0; i < this.buttons.Count; i++) {
			IconButton button = this.buttons[i];
			float yOffset = i * rowHeight;

			Rect hoverRect = Rect.FromSize(0.31f, startY - yOffset, 0.59f, 0.05f);
			Rect fillRect = Rect.FromSize(0.305f, startY - 0.005f - yOffset, 0.5905f, 0.06f);

			if (hoverRect.Inside(Mouse.X, Mouse.Y)) {
				Immediate.Color(0.25f, 0.25f, 0.25f);
				UI.FillRect(fillRect);

				if (Mouse.JustLeft) {
					if (button.callback?.Invoke() ?? false) {
						this.Close();
					}
					return;
				}
			}

			Immediate.UseTexture(this.uiIcons);
			Program.gl.Enable(EnableCap.Blend);
			Immediate.Color(1f, 1f, 1f);

			UI.FillRect(UVRect.FromSize(0.31f, startY - yOffset, 0.05f, 0.05f).UV(button.UVs.u1, button.UVs.v1, button.UVs.u2, button.UVs.v2));

			Immediate.UseTexture(0);
			Program.gl.Disable(EnableCap.Blend);

			UI.font.Write(button.label, 0.37f, startY + 0.025f - yOffset, 0.03f, Font.Align.MiddleLeft);
		}

		if (Mouse.JustLeft) {
			this.Close();

			if (!Settings.HideTutorial) {
				PopupManager.Add(new MarkdownPopup("docs/TutorialWorld.md"));
			}
		}
	}

	protected class IconButton {
		public string label;
		public (float u1, float v1, float u2, float v2) UVs;
		public Func<bool> callback;

		public IconButton(string label, float u1, float v1, float u2, float v2, Func<bool> callback) {
			this.label = label;
			this.UVs = (u1, v1, u2, v2);
			this.callback = callback;
		}
	}
}