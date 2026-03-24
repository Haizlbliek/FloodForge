using System.Text.RegularExpressions;
using Silk.NET.Input;
using Silk.NET.SDL;

// TODO: Select by absolute paths, not the filename

namespace FloodForge.Popups;

public class FilesystemPopup : Popup {
	protected static List<string> previousDirectories = [ Settings.DefaultFilePath ];

	protected Regex? regexFilter = null;
	protected bool allowMultiple = false;
	protected SelectionType typeFilter = SelectionType.File;
	protected string hint = "";

	protected string? newDirectory = null;
	protected bool showAll = false;
	protected float scroll;
	protected float targetScroll;
	protected int frame;
	protected bool awaitingDeleteConfirmation = false;
	protected bool calledCallback = false;
	protected Action<string[]> callback;
	protected HashSet<string> selected = [];
	protected string[] directories = [];
	protected string[] files = [];
	protected List<string> createdFolders = [];
	protected string currentPath;
	protected int directoryIndex = 0;
	protected Dictionary<string, string> rootPaths = [];
	protected float fontSize = 0.04f;

	public FilesystemPopup(Action<string[]> callback, int directoryIndex = 0) : base() {
		this.directoryIndex = directoryIndex;
		while (previousDirectories.Count <= directoryIndex) {
			previousDirectories.Add("");
		}

		this.callback = callback;
		Main.Scroll += this.Scroll;
		Main.KeyPress += this.KeyPress;
		this.currentPath = "";
		this.SetupDirectory();
		this.Refresh();
	}

	protected void SetupDirectory() {
		string previousDirectory = previousDirectories[this.directoryIndex];
		if (previousDirectory != "" && Directory.Exists(previousDirectory)) {
			this.currentPath = previousDirectory;
			return;
		}

		if (Settings.DefaultFilePath != "" && Directory.Exists(Settings.DefaultFilePath)) {
			this.currentPath = Settings.DefaultFilePath;
			return;
		}

		string? homePath = Environment.GetEnvironmentVariable("HOME");
		if (homePath != null) {
			string path = Path.Join(homePath, ".steam/steam/steamapps/common/Rain World/RainWorld_Data/StreamingAssets");
			if (Directory.Exists(path)) {
				this.currentPath = path;
				return;
			}
		}

		string relativePath = Path.Combine("Program Files (x86)", "Steam", "steamapps", "common", "Rain World", "RainWorld_Data", "StreamingAssets");
		foreach (DriveInfo drive in DriveInfo.GetDrives()) {
			if (!drive.IsReady) continue;

			string fullPath = Path.Combine(drive.RootDirectory.FullName, relativePath);
			if (Directory.Exists(fullPath)) {
				this.currentPath = fullPath;
				return;
			}
		}
	}

	public FilesystemPopup Filter(Regex filter) {
		this.regexFilter = filter;
		this.Refresh();
		return this;
	}

	public FilesystemPopup Filter(SelectionType filter) {
		this.typeFilter = filter;
		this.Refresh();
		return this;
	}

	public FilesystemPopup Hint(string hint) {
		this.hint = hint;
		return this;
	}

	public FilesystemPopup Multiple() {
		this.allowMultiple = true;
		return this;
	}

	protected void Refresh() {
		this.selected.Clear();
		List<string> f = [];
		List<string> d = [];

		foreach (string entry in Directory.EnumerateFileSystemEntries(this.currentPath).Order()) {
			if (Directory.Exists(entry)) {
				d.Add(Path.GetFileName(entry));
				continue;
			}

			if (this.showAll || (this.regexFilter == null) || this.regexFilter.IsMatch(Path.GetFileName(entry))) {
				f.Add(Path.GetFileName(entry));
			}
		}

		this.files = [.. f];
		this.directories = [.. d];
	}

	protected void ClampScroll() {
		int size = this.directories.Length + this.files.Length;

		if (this.targetScroll < -size * 0.06f + 0.06f) {
			this.targetScroll = -size * 0.06f + 0.06f;
			if (this.scroll <= -size * 0.06f + 0.12f) {
				this.scroll = -size * 0.06f + 0.03f;
			}
		}

		if (this.targetScroll > 0f) {
			this.targetScroll = 0f;
			if (this.scroll >= -0.06f) {
				this.scroll = 0.03f;
			}
		}
	}

	protected void Scroll(float x, float y) {
		if (!this.hovered || this.minimized) return;

		this.targetScroll += y * 0.06f;
		this.ClampScroll();
	}


	protected void DrawIcon(int type, float y) {
		this.DrawIcon(type, this.bounds.x0 + 0.02f, y);
	}

	protected void DrawIcon(int type, float x, float y) {
		Program.gl.Enable(EnableCap.Blend);
		Immediate.UseTexture(UI.ui);
		Immediate.Begin(Immediate.PrimitiveType.QUADS);

		float offsetUVx = type % 4 * 0.25f;
		float offsetUVy = type / 4 * 0.25f;

		Immediate.TexCoord(0.00f + offsetUVx, 0.00f + offsetUVy); Immediate.Vertex(x + 0.00f, y - 0.00f);
		Immediate.TexCoord(0.25f + offsetUVx, 0.00f + offsetUVy); Immediate.Vertex(x + 0.05f, y - 0.00f);
		Immediate.TexCoord(0.25f + offsetUVx, 0.25f + offsetUVy); Immediate.Vertex(x + 0.05f, y - 0.05f);
		Immediate.TexCoord(0.00f + offsetUVx, 0.25f + offsetUVy); Immediate.Vertex(x + 0.00f, y - 0.05f);

		Immediate.End();
		Immediate.UseTexture(0);
		Program.gl.Disable(EnableCap.Blend);
	}



	public override void Accept() {
		previousDirectories[this.directoryIndex] = this.currentPath;

		if (this.newDirectory != null) {
			string newDirectoryPath = Path.Join(this.currentPath, this.newDirectory);
			if (this.newDirectory != "" && Path.Exists(newDirectoryPath)) {
				this.newDirectory = null;
				return;
			}

			Directory.CreateDirectory(newDirectoryPath);
			this.createdFolders.Add(newDirectoryPath);
			this.newDirectory = null;
			this.Refresh();
			this.ClampScroll();
			return;
		}

		this.calledCallback = true;
		if (this.typeFilter == SelectionType.Folder) {
			this.callback([this.currentPath]);
		}
		else {
			this.callback([.. this.selected.Select(x => Path.Combine(this.currentPath, x))]);
		}

		this.Close();
	}

	public override void Reject() {
		if (this.newDirectory != null) {
			this.newDirectory = null;
			return;
		}

		this.Close();
	}

	public override void Close() {
		Main.Scroll -= this.Scroll;
		Main.KeyPress -= this.KeyPress;

		base.Close();
	
		if (!this.calledCallback) {
			this.callback([]);
		}
	}

	protected void KeyPress(Key key) {
		if (this.newDirectory == null) return;

		if (key == Key.V && Keys.Modifier(Keymod.Ctrl)) {
			this.newDirectory += Clipboard.Get().Replace("\\", "").Replace("/", "").Replace(".", "");
			this.frame = 0;
			return;
		}

		if (key == Key.Period || key == Key.Slash || key == Key.BackSlash || key == Key.Enter) {
			return;
		}

		if (key == Key.Backspace) {
			if (this.newDirectory.Length > 0) this.newDirectory = this.newDirectory[0..^1];
			this.frame = 0;
			return;
		}

		if (key >= Key.A && key <= Key.Z) {
			this.frame = 0;
			string k = key.ToString();
			this.newDirectory += Keys.Modifier(Keymod.Shift) ? k.ToUpper() : k.ToLower();
		}

		if (key >= Key.Number0 && key <= Key.Number9) {
			this.frame = 0;
			this.newDirectory += key.ToString()[^1];
		}
	}

	public void DeleteFolder(string path) {
		Directory.Delete(path, true);
		this.createdFolders.Remove(path);
		this.Refresh();
	}

	public override void Draw() {
		base.Draw();

		if (this.minimized) return;

		this.scroll += (this.targetScroll - this.scroll) * (1f - MathF.Pow(1f - Settings.PopupScrollSpeed, Program.Delta * 60f));
		this.frame++;


		// Up Directory
		if (UI.TextureButton(UVRect.FromSize(this.bounds.x0 + 0.02f, this.bounds.y1 - 0.12f, 0.05f, 0.05f).UV(0.25f, 0.25f, 0.5f, 0f))) {
			this.currentPath = PathUtil.Parent(this.currentPath);
			this.scroll = 0f;
			this.targetScroll = 0f;
			this.Refresh();
			this.ClampScroll();
		}

		// Refresh
		if (UI.TextureButton(UVRect.FromSize(this.bounds.x0 + 0.09f, this.bounds.y1 - 0.12f, 0.05f, 0.05f).UV(0.5f, 0.25f, 0.75f, 0f))) {
			this.Refresh();
			this.ClampScroll();
		}

		// New Directory
		if (UI.TextureButton(UVRect.FromSize(this.bounds.x1 - 0.09f, this.bounds.y1 - 0.12f, 0.05f, 0.05f).UV(0.25f, 0.5f, 0.5f, 0.25f))) {
			this.newDirectory = "";
			this.scroll = 0f;
			this.targetScroll = 0f;
		}

		if (this.typeFilter == SelectionType.File) {
			if (UI.CheckBox(Rect.FromSize(this.bounds.x0 + 0.02f, this.bounds.y0 + 0.04f, 0.05f, 0.05f), ref this.showAll)) {
				this.Refresh();
				this.ClampScroll();
			}

			Immediate.Color(Themes.Text);
			UI.font.Write("Show all", this.bounds.x0 + 0.09f, this.bounds.y0 + 0.09f, this.fontSize);

			Immediate.Color(Themes.TextDisabled);
			UI.font.Write(this.hint, this.bounds.x0 + 0.35f, this.bounds.y0 + 0.09f, this.fontSize);
		}
		else {
			Immediate.Color(Themes.TextDisabled);
			UI.font.Write(this.hint, this.bounds.x0 + 0.02f, this.bounds.y0 + 0.09f, this.fontSize);
		}

		if (UI.TextButton("Open", new Rect(this.bounds.x1 - 0.16f, this.bounds.y0 + 0.09f, this.bounds.x1 - 0.05f, this.bounds.y0 + 0.04f), new UI.TextButtonMods() { disabled = this.selected.Count == 0 && this.typeFilter == SelectionType.File })) {
			this.Accept();
		}

		string croppedPath = Font.CropText(this.currentPath, this.bounds.x1 - 0.11f - (this.bounds.x0 + 0.23f), out _, true);

		Immediate.Color(Themes.Text);
		UI.font.Write(croppedPath, this.bounds.x0 + 0.23f, this.bounds.y1 - 0.07f, this.fontSize);

		if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows)) {
			string root = Path.GetPathRoot(this.currentPath) ?? "";
			if (UI.TextButton(root, new Rect(this.bounds.x0 + 0.16f, this.bounds.y1 - 0.12f, this.bounds.x0 + 0.21f, this.bounds.y1 - 0.07f))) {
				DriveInfo[] drives = [.. DriveInfo.GetDrives().Where(d => d.IsReady)];
				if (drives.Length > 0) {
					int idx = Array.FindIndex(drives, d => d.Name.Equals(root, StringComparison.InvariantCultureIgnoreCase));
					string newRoot = drives[(idx + 1) % drives.Length].Name;
					this.rootPaths[root] = this.currentPath;
					this.currentPath = Path.Combine(newRoot, this.currentPath[root.Length..]);
					if (this.rootPaths.TryGetValue(newRoot, out string? oldPath))
						this.currentPath = oldPath;
					if (!Directory.Exists(this.currentPath)) {
						this.currentPath = newRoot;
					}
					this.Refresh();
				}
			}
		}


		float offsetY = this.bounds.CenterY;
		float scrollAreaY1 = this.bounds.y1 - 0.135f;
		float scrollAreaY0 = this.bounds.y0 + 0.2f;
		float y = scrollAreaY1 - this.scroll;
		bool hasExtras = false;

		if (this.newDirectory != null) {
			if (y > scrollAreaY0) {
				if (y > scrollAreaY1) {
					y -= 0.06f;
				}
				else {
					if(UI.TextureButton(new UVRect(this.bounds.x1 - 0.09f, y - 0.05f, this.bounds.x1 - 0.04f, y).UV(0f, 0f, 0.25f, 0.25f))) {
						this.newDirectory = null;
					}
					else {
						Immediate.Color(Themes.TextDisabled);
						UI.FillRect(this.bounds.x0 + 0.1f, y, this.bounds.x1 - 0.1f, y - 0.05f);

						Immediate.Color(Themes.TextHighlight);
						string cropText = Font.CropText(this.newDirectory, this.bounds.x1 - this.bounds.x0 - 0.1f, out float margin, true);
						margin = (cropText.Length != this.newDirectory.Length ? margin : 0f);
						UI.font.Write(cropText, margin + this.bounds.x0 + 0.1f, y, this.fontSize);

						if (this.frame % 60 < 30) {
							Immediate.Color(Themes.Text);
							float cursorX = margin + this.bounds.x0 + 0.1f + UI.font.Measure(cropText, 0.04f).x;
							UI.FillRect(cursorX, y + 0.01f, cursorX + 0.005f, y - 0.06f);
						}

						Immediate.Color(Themes.TextDisabled);
						this.DrawIcon(5, y);
						y -= 0.06f;
					}
				}
			}
		}

		bool refreshing = false;

		foreach (string path in this.directories) {
			if (y <= scrollAreaY0) { hasExtras = true; break; }
			if (y > scrollAreaY1) {
				y -= 0.06f;
				continue;
			}

			Rect rect = new Rect(this.bounds.x0 + 0.1f, y, this.bounds.x1 - 0.1f, y - 0.06f);
			bool hover = rect.Inside(Mouse.X, Mouse.Y) &! this.awaitingDeleteConfirmation;

			Immediate.Color(hover ? Themes.TextHighlight : Themes.Text);
			UI.font.Write(path + "/", this.bounds.x0 + 0.1f, y, this.fontSize);
			string currentFolderPath = Path.Join(this.currentPath, path);
			if (this.createdFolders.Contains(currentFolderPath)) {
				if (UI.TextureButton(new UVRect(this.bounds.x1 - 0.09f, y - 0.05f, this.bounds.x1 - 0.04f, y).UV(0f, 0f, 0.25f, 0.25f))) {
					if (Directory.GetFileSystemEntries(currentFolderPath).Length != 0) {
						this.awaitingDeleteConfirmation = true;
						PopupManager.Add(new ConfirmPopup("Delete folder?").SetButtons("Delete", "Cancel").Swap().Okay(() => { this.DeleteFolder(currentFolderPath); this.awaitingDeleteConfirmation = false; }).Cancel(() => { this.awaitingDeleteConfirmation = false; }));
					}
					else {
						this.DeleteFolder(currentFolderPath);
					}
				}
			}

			Immediate.Color(Themes.TextDisabled);
			this.DrawIcon(5, y);

			if (hover && Mouse.JustLeft) {
				this.currentPath = currentFolderPath;
				this.scroll = 0f;
				this.targetScroll = 0f;
				this.Refresh();
				refreshing = true;
				break;
			}

			y -= 0.06f;
		}

		foreach (string path in this.files) {
			if (refreshing) break;
			if (y <= scrollAreaY0) { hasExtras = true; break; }
			if (y > scrollAreaY1) {
				y -= 0.06f;
				continue;
			}

			Rect rect = new Rect(this.bounds.x0 + 0.1f, y, this.bounds.x1 - 0.1f, y - 0.06f);
			bool hover = rect.Inside(Mouse.X, Mouse.Y);

			if (hover && Mouse.JustLeft) {
				if (this.allowMultiple && Keys.Modifier(Keymod.Shift) || Keys.Modifier(Keymod.Ctrl)) {
					if (Keys.Modifier(Keymod.Shift)) {
						string latestSelected = this.selected.Last();
						bool startSelecting = false;
						foreach (string selectPath in this.files) {
							if (selectPath == latestSelected || selectPath == path) {
								startSelecting = !startSelecting;
							}
							if (startSelecting || selectPath == latestSelected || selectPath == path) {
								this.selected.Add(selectPath);
							}
						}
					}
					else if (Keys.Modifier(Keymod.Ctrl)) {
						if (!this.selected.Remove(path)) {
							this.selected.Add(path);
						}
					}
				}
				else {
					this.selected.Clear();
					this.selected.Add(path);
				}
			}

			Immediate.Color(hover ? Themes.TextHighlight : Themes.Text);
			if (this.selected.Contains(path)) {
				UI.StrokeRect(this.bounds.x0 + 0.09f, y + 0.01f, this.bounds.x1 - 0.09f, y - 0.05f);
			}
			UI.font.Write(path, this.bounds.x0 + 0.1f, y, this.fontSize);

			Immediate.Color(Themes.TextDisabled);
			this.DrawIcon(4, y);

			y -= 0.06f;
		}

		if (hasExtras && !refreshing) {
			Immediate.Color(Themes.TextDisabled);
			//UI.font.Write("...", this.bounds.x0 + 0.1f, MathF.Ceiling((y - offsetY) / 0.06f) * 0.06f + offsetY, 0.04f);
			UI.font.Write("...", this.bounds.x0 + 0.1f, y, this.fontSize);
		}
	}

	public enum SelectionType {
		File,
		Folder
	}
}