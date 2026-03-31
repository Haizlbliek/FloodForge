using System.Diagnostics;
using System.Net.Http;
using System.Text.Json.Nodes;
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
		this.splashArt = Texture.Load(Main.AprilFools ? "assets/objects/splash-corrupted.png" : "assets/splash.png");
		this.uiIcons = Texture.Load("assets/uiIcons.png");

		this.version = new AppVersion(File.ReadAllText("assets/version.txt"));
		if (File.Exists("assets/nightly.txt") && DateTime.TryParse(File.ReadAllText("assets/nightly.txt").Trim(), out DateTime date)) {
			this.nightlyBuildDate = date;
		}
		this.CheckForUpdates();
		this.buttons.Add(new IconButton("Discord Server", 0f, 0f, 0.25f, 0.25f, () => {
			Process.Start(new ProcessStartInfo() { FileName = "https://discord.gg/k5BExadp4x", UseShellExecute = true });
		}));

		if (Main.Anniversary) {
			this.buttons.Add(new IconButton("Anniversary Event", 0.25f, 0f, 0.5f, 0.25f, () => {
				PopupManager.Add(new MarkdownPopup("docs/anniversary.md"));
			}));
		}
	}

	private async void CheckForUpdates() {
		try {
			HttpClient client = new HttpClient();
			client.DefaultRequestHeaders.Add("User-Agent", "FloodForge-Updater");
			string url = this.nightlyBuildDate == null ? "https://api.github.com/repos/Haizlbliek/FloodForge/releases/latest" : "https://api.github.com/repos/Haizlbliek/FloodForge/releases/tags/nightly";
			Stream response = await client.GetStreamAsync(url);
			JsonNode? node = await JsonNode.ParseAsync(response);
			if (node == null) {
				Logger.Warn("Failed to fetch release version");
				this.updateStatus = UpdateStatus.Failed;
				return;
			}
			if (this.nightlyBuildDate == null) {
				JsonNode? node2 = node["tag_name"];
				if (node2 == null) {
					Logger.Warn("Failed to fetch release version");
					this.updateStatus = UpdateStatus.Failed;
					return;
				}
				string? value = (string?) node2;
				if (value == null) {
					Logger.Warn("Failed to fetch release version");
					this.updateStatus = UpdateStatus.Failed;
					return;
				}
				AppVersion latest = new AppVersion(value);
				this.updateStatus = (this.version < latest) ? UpdateStatus.Available : UpdateStatus.Unavailable;
			}
			else {
				string? body = node["body"]?.ToString().Replace("BUILD_DATE: ", "").Trim();
				if (body == null || !DateTime.TryParse(body, out DateTime date)) {
					Logger.Warn("Failed to fetch release body");
					this.updateStatus = UpdateStatus.Failed;
					return;
				}

				this.updateStatus = (this.nightlyBuildDate < date) ? UpdateStatus.Available : UpdateStatus.Unavailable;
			}

			if (this.updateStatus == UpdateStatus.Available) {
				string targetFileName = RuntimeInformation.IsOSPlatform(OSPlatform.Windows) ? "FloodForge-Windows.zip" : "FloodForge-Linux.zip";
				JsonArray? assets = node["assets"]?.AsArray();
				string? downloadUrl = null;
				if (assets != null) {
					foreach (var asset in assets) {
						if (asset?["name"]?.ToString().Equals(targetFileName, StringComparison.InvariantCultureIgnoreCase) ?? false) {
							downloadUrl = asset["browser_download_url"]?.ToString();
							string? digest = asset["digest"]?.ToString();
							this.updateChecksum = digest?.Replace("sha256:", "");
							break;
						}
					}
				}

				if (this.updateChecksum == null) {
					Logger.Warn("Failed to fetch latest release checksum");
					this.updateStatus = UpdateStatus.Failed;
				}

				if (!string.IsNullOrEmpty(downloadUrl)) {
					this.updatePath = downloadUrl;

					this.buttons.Add(new IconButton("Update Available!", 0.5f, 0f, 0.75f, 0.25f, () => {
						PopupManager.Add(new ConfirmPopup($"Update to latest {(this.nightlyBuildDate == null ? "version" : "nightly")}?").Okay(async () => {
							PopupManager.Add(new InfoPopup("Downloading, please wait."));
							Logger.Note($"Downloading {this.updatePath} {this.updateChecksum}");
							await Updater.Download(this.updatePath, this.updateChecksum!);
							Logger.Note($"Downloaded");
						}));
					}));
				}
				else {
					Logger.Warn($"Could not find a release asset for {targetFileName}");
				}
			}
		} catch (HttpRequestException) {
			Logger.Warn("Failed to fetch latest release version");
			this.updateStatus = UpdateStatus.Failed;
		} catch (Exception ex) {
			Logger.Warn("Failed to fetch latest release version");
			Logger.Info(ex);
			this.updateStatus = UpdateStatus.Failed;
		}
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
			if (i >= RecentFiles.recents.Count) break;

			string recent = RecentFiles.recentNames[i];
			if (recent.IsNullOrEmpty()) {
				recent = Path.GetFileNameWithoutExtension(RecentFiles.recents[i]);
			}

			float y = -0.33f - i * 0.04f;
			Rect rect = new Rect(-0.89f, y - 0.02f, -0.4f, y + 0.015f);

			if (rect.Inside(Mouse.X, Mouse.Y)) {
				Immediate.Color(0.25f, 0.25f, 0.25f);
				UI.FillRect(rect);

				if (Mouse.JustLeft) {
					this.Close();
					WorldParser.ImportWorldFile(RecentFiles.recents[i]);
					return;
				}
			}

			Immediate.Color(1f, 1f, 1f);
			UI.font.Write(recent, -0.88f, y, 0.03f, Font.Align.MiddleLeft);
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
					this.Close();
					button.callback?.Invoke();
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
		public Action callback;

		public IconButton(string label, float u1, float v1, float u2, float v2, Action callback) {
			this.label = label;
			this.UVs = (u1, v1, u2, v2);
			this.callback = callback;
		}
	}
}