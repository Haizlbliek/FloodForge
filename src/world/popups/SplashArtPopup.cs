using System.Diagnostics;
using FloodForge.Popups;
using Stride.Core.Extensions;

namespace FloodForge.World;

public class SplashArtPopup : Popup {
	protected Texture splashArt;
	protected Texture uiIcons;
	protected string version;

	public SplashArtPopup() {
		this.bounds = new Rect(-1f, -1f, 1f, 1f);
		this.splashArt = Texture.Load(Main.AprilFools ? "assets/objects/splash-corrupted.png" : "assets/splash.png");
		this.uiIcons = Texture.Load("assets/uiIcons.png");

		this.version = File.ReadAllText("assets/version.txt");
		if (Main.AprilFools) {
			this.version = this.version.Replace("beta", "zeta").Replace("1", "42").Replace("6", "9831").Replace("5", "51");
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
		UI.FillRect(-0.9f, -0.65f, 0.9f, 0.65f);

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
		UI.font.Write(this.version, -0.88f, 0.63f, 0.04f);
	
		Immediate.Color(0.8f, 0.8f, 0.8f);
		UI.font.Write("Recent worlds:", -0.88f, -0.28f, 0.03f, Font.Align.MiddleLeft);

		for (int i = 0; i < 8; i++) {
			int revIndex = Math.Min(RecentFiles.recents.Count, 8) - 1 - i;
			if (revIndex < 0) break;

			string recent = RecentFiles.recentNames[revIndex];
			if (recent.IsNullOrEmpty()) {
				recent = Path.GetFileNameWithoutExtension(RecentFiles.recents[revIndex]);
			}

			float y = -0.33f - i * 0.04f;
			Rect rect = new Rect(-0.89f, y - 0.02f, -0.4f, y + 0.015f);

			if (rect.Inside(Mouse.X, Mouse.Y)) {
				Immediate.Color(0.25f, 0.25f, 0.25f);
				UI.FillRect(rect);

				if (Mouse.JustLeft) {
					this.Close();
					WorldParser.ImportWorldFile(RecentFiles.recents[revIndex]);
					return;
				}
			}

			Immediate.Color(1f, 1f, 1f);
			UI.font.Write(recent, -0.88f, y, 0.03f, Font.Align.MiddleLeft);
		}

		UI.StrokeRect(-0.9f, -0.65f, 0.9f, 0.65f);
		UI.Line(-0.9f, -0.25f, 0.9f, -0.25f);

		for (int i = 0; i < (Main.Anniversary ? 2 : 1); i++) {
			Rect hoverRect = Rect.FromSize(0.31f, -0.31f - i * 0.06f, 0.59f, 0.05f);
			Rect rect = Rect.FromSize(0.305f, -0.315f - i * 0.06f, 0.5905f, 0.06f);
			if (hoverRect.Inside(Mouse.X, Mouse.Y)) {
				Immediate.Color(0.25f, 0.25f, 0.25f);
				UI.FillRect(rect);

				if (Mouse.JustLeft) {
					this.Close();
					if (i == 0) {
						Process.Start(new ProcessStartInfo() { FileName = "https://discord.gg/k5BExadp4x", UseShellExecute = true });
					}
					else if (i == 1) {
						PopupManager.Add(new MarkdownPopup("docs/anniversary.md"));
					}
				}
			}
		}

		Immediate.UseTexture(this.uiIcons);
		Program.gl.Enable(EnableCap.Blend);
		Immediate.Color(1f, 1f, 1f);

		UI.FillRect(UVRect.FromSize(0.31f, -0.31f, 0.05f, 0.05f).UV(0f, 0f, 0.25f, 0.25f));
		if (Main.Anniversary) UI.FillRect(UVRect.FromSize(0.31f, -0.37f, 0.05f, 0.05f).UV(0.25f, 0f, 0.5f, 0.25f));

		Immediate.UseTexture(0);
		Program.gl.Disable(EnableCap.Blend);

		UI.font.Write("Discord Server", 0.37f, -0.285f, 0.03f, Font.Align.MiddleLeft);
		if (Main.Anniversary) UI.font.Write("Anniversary Event", 0.37f, -0.345f, 0.03f, Font.Align.MiddleLeft);

		if (Mouse.JustLeft) {
			this.Close();

			if (Settings.HideTutorial) {
				PopupManager.Add(new MarkdownPopup("docs/controls.md"));
			}
		}
	}
}