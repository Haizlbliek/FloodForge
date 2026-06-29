using System.Text.RegularExpressions;
using Silk.NET.Input;

// TODO: Select by absolute paths, not the filename

namespace FloodForge.Popups;

public class FilesystemPopup : Popup {
	protected static List<string> previousDirectories = [ Settings.DefaultFilePath ];

	protected Regex? regexFilter = null;
	protected bool allowMultiple = false;
	protected SelectionType typeFilter = SelectionType.File;
	protected string hint = "";

	protected string? search = null;
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
	protected bool noDirectoryPerms;

	public FilesystemPopup(Action<string[]> callback, int directoryIndex = 0) : base() {
		this.directoryIndex = directoryIndex;
		while (previousDirectories.Count <= directoryIndex) {
			previousDirectories.Add("");
		}

		this.callback = callback;
		Main.Scroll += this.Scroll;
		Main.KeyPress += this.KeyPress;
		this.currentPath = "";
		try {
			this.SetupDirectory();
			this.Refresh();
		}
		catch (Exception e) {
			PopupManager.Add(new InfoPopup("Unable to open Filesystem!\nError message:\n" + e.Message));
			this.Close();
		}
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
			string officialPath = Path.Join(homePath, ".steam/steam/steamapps/common/Rain World/RainWorld_Data/StreamingAssets");
			if (Directory.Exists(officialPath)) {
				this.currentPath = officialPath;
				return;
			}
			string flatpakPath = Path.Join(homePath, ".var/app/com.valvesoftware.Steam/.local/share/Steam/steamapps/common/Rain World/RainWorld_Data/StreamingAssets");
			if (Directory.Exists(flatpakPath)) {
				this.currentPath = flatpakPath;
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

		Logger.Warn("No Rain World installation detected!");
		foreach (DriveInfo drive in DriveInfo.GetDrives()) {
			if (!drive.IsReady) continue;

			this.currentPath = drive.RootDirectory.FullName;
			return;
		}
		throw new DirectoryNotFoundException("No ready drives detected!");
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
		List<string> d = [];
		List<string> f = [];
		this.noDirectoryPerms = false;

		string? trimmedSearch = this.search?.TrimStart();
		bool hasSearch = !string.IsNullOrEmpty(trimmedSearch);

		string[] searchWords = hasSearch ? trimmedSearch!.Split([' ', '-', '_', '.'], StringSplitOptions.RemoveEmptyEntries) : [];

		try {
			foreach (string entry in Directory.EnumerateFileSystemEntries(this.currentPath).Order()) {
				string name = Path.GetFileName(entry);

				if (hasSearch) {
					bool matchesLiteral = name.Contains(trimmedSearch!, StringComparison.InvariantCultureIgnoreCase);
					bool matchesFuzzy = false;

					if (!matchesLiteral && searchWords.Length > 0) {
						int lastIndex = 0;
						bool sequenceMatched = true;

						foreach (string word in searchWords) {
							int foundIndex = name.IndexOf(word, lastIndex, StringComparison.InvariantCultureIgnoreCase);
							if (foundIndex == -1) {
								sequenceMatched = false;
								break;
							}
							lastIndex = foundIndex + word.Length;
						}
						matchesFuzzy = sequenceMatched;
					}

					if (!matchesLiteral && !matchesFuzzy) {
						continue;
					}
				}

				if (Directory.Exists(entry)) {
					d.Add(name);
					continue;
				}

				if (this.showAll || this.regexFilter == null || this.regexFilter.IsMatch(name)) {
					f.Add(name);
				}
			}
		} catch (UnauthorizedAccessException) {
			this.noDirectoryPerms = true;
		} catch (Exception ex) {
			Logger.Error(ex.ToString());
			throw;
		} 

		if (hasSearch && trimmedSearch != null) {
			int getSortIndex(string s) {
				int index = s.IndexOf(trimmedSearch, StringComparison.InvariantCultureIgnoreCase);
				if (index != -1) return index;

				if (searchWords.Length > 0) {
					index = s.IndexOf(searchWords[0], StringComparison.InvariantCultureIgnoreCase);
					if (index != -1) return index;
				}

				return int.MaxValue;
			}

			d = [.. d.OrderBy(getSortIndex)];
			f = [.. f.OrderBy(getSortIndex)];
		}

		this.directories = [.. d];
		this.files = [.. f];
		this.ClampScroll();
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
		if (!this.isHovered || this.collapsed) return;

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
			return;
		}

		if (this.search != null) {
			if (this.directories.Length > 0) {
				this.currentPath = PathUtil.Combine(this.currentPath, this.directories[0]);
				this.search = null;
				this.Refresh();
				return;
			}
		}

		this.calledCallback = true;
		if (this.typeFilter == SelectionType.Folder) {
			this.callback([this.currentPath]);
		}
		else if (this.selected.Count > 0) {
			this.callback([.. this.selected.Select(x => Path.Combine(this.currentPath, x))]);
		}
		else {
			return;
		}

		previousDirectories[this.directoryIndex] = this.currentPath;
		this.Close();
	}

	public override void Reject() {
		if (this.newDirectory != null) {
			this.newDirectory = null;
			return;
		}

		if (this.search != null) {
			this.search = null;
			this.Refresh();
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
		bool isNewDirActive = this.newDirectory != null;

		string currentText = (isNewDirActive ? this.newDirectory : this.search) ?? "";

		if (key == Key.Backspace && this.newDirectory == null && this.search == null) {
			this.currentPath = PathUtil.Parent(this.currentPath);
			this.Refresh();
			return;
		}

		if (key == Key.V && Keys.Modifier(Keys.Modifiers.Control)) {
			string clipboard = Clipboard.Content ?? "";

			char[] invalidChars = Path.GetInvalidFileNameChars();
			string clipboardClean = new string(clipboard.Where(c => !invalidChars.Contains(c)).ToArray());

			currentText += clipboardClean;
			this.frame = 0;

			if (isNewDirActive) {
				this.newDirectory = currentText;
			}
			else {
				this.search = currentText;
				this.Refresh();
			}
			return;
		}

		if (key == Key.Enter || key == Key.Escape || key == Key.Backspace) {
			if (key == Key.Backspace) {
				if (currentText.Length > 0) currentText = currentText[0..^1];
				this.frame = 0;

				if (isNewDirActive) {
					this.newDirectory = currentText;
				}
				else {
					this.search = currentText;
					this.Refresh();
				}
			}
			return;
		}

		bool isShiftPressed = Keys.Modifier(Keys.Modifiers.Shift);
		string? appendedString = null;

		if (key >= Key.A && key <= Key.Z) {
			string k = key.ToString();
			appendedString = isShiftPressed ? k.ToUpper() : k.ToLower();
		}
		else if (key >= Key.Number0 && key <= Key.Number9) {
			char numChar = key.ToString()[^1];
			if (isShiftPressed) {
				appendedString = numChar switch {
					'1' => "!", '2' => "@", '3' => "#", '4' => "$", '5' => "%",
					'6' => "^", '7' => "&", '8' => "*", '9' => "(", '0' => ")",
					_ => numChar.ToString()
				};
			} else {
				appendedString = numChar.ToString();
			}
		}
		else if (key >= Key.Keypad0 && key <= Key.Keypad9) {
			appendedString = key.ToString()[^1].ToString();
		}
		else {
			appendedString = key switch {
				Key.Space => " ",
				Key.Minus => isShiftPressed ? "_" : "-",
				Key.KeypadSubtract => "-",
				Key.Equal => isShiftPressed ? "+" : "=",
				Key.KeypadAdd => "+",
				Key.Comma => isShiftPressed ? "<" : ",",
				Key.Semicolon => isShiftPressed ? ":" : ";",
				Key.Apostrophe => isShiftPressed ? "\"" : "'",
				Key.GraveAccent => isShiftPressed ? "~" : "`",
				Key.LeftBracket => isShiftPressed ? "{" : "[",
				Key.RightBracket => isShiftPressed ? "}" : "]",
				_ => null
			};
		}

		if (!string.IsNullOrEmpty(appendedString)) {
			char[] invalidFileNameChars = Path.GetInvalidFileNameChars();

			string validatedAppend = new string(appendedString.Where(c => !invalidFileNameChars.Contains(c)).ToArray());

			if (validatedAppend.Length > 0) {
				this.frame = 0;
				currentText += validatedAppend;
			}
		}

		if (isNewDirActive) {
			this.newDirectory = currentText;
		}
		else {
			this.search = currentText;
			this.Refresh();
		}
	}

	public void DeleteFolder(string path) {
		Directory.Delete(path, true);
		this.createdFolders.Remove(path);
		this.Refresh();
	}


	public override void Draw() {
		base.Draw();

		if (this.collapsed) return;

		this.scroll += (this.targetScroll - this.scroll) * (1f - MathF.Pow(1f - Settings.PopupScrollSpeed, Program.Delta * 60f));
		this.frame++;


		// Up Directory
		if (UI.TextureButton(UVRect.FromSize(this.bounds.x0 + 0.01f, this.bounds.y1 - 0.11f, 0.05f, 0.05f).UV(0.25f, 0.25f, 0.5f, 0f))) {
			this.currentPath = PathUtil.Parent(this.currentPath);
			this.scroll = 0f;
			this.targetScroll = 0f;
			this.Refresh();
		}

		// Refresh
		if (UI.TextureButton(UVRect.FromSize(this.bounds.x0 + 0.07f, this.bounds.y1 - 0.11f, 0.05f, 0.05f).UV(0.5f, 0.25f, 0.75f, 0f))) {
			this.Refresh();
		}

		// Search
		if (UI.TextureButton(UVRect.FromSize(this.bounds.x1 - 0.12f, this.bounds.y1 - 0.11f, 0.05f, 0.05f).UV(0.25f, 1f, 0.5f, 0.75f), new UI.TextureButtonMods() { disabled = this.newDirectory != null || this.noDirectoryPerms == true})) {
			this.search = this.search == null ? "" : null;
			this.Refresh();
		}

		// New Directory
		if (UI.TextureButton(UVRect.FromSize(this.bounds.x1 - 0.06f, this.bounds.y1 - 0.11f, 0.05f, 0.05f).UV(0.25f, 0.5f, 0.5f, 0.25f), new UI.TextureButtonMods() { disabled = this.search != null || this.noDirectoryPerms == true})) {
			if (this.newDirectory == null) {
				this.newDirectory = "";
				this.scroll = 0f;
				this.targetScroll = 0f;
			}
			else {
				this.newDirectory = null;
			}
		}

		if (this.typeFilter == SelectionType.File) {
			if (UI.CheckBox(Rect.FromSize(this.bounds.x0 + 0.01f, this.bounds.y0 + 0.01f, 0.05f, 0.05f), ref this.showAll)) {
				this.Refresh();
			}

			Immediate.Color(Themes.Text);
			UI.font.Write("Show all", this.bounds.x0 + 0.08f, this.bounds.y0 + 0.06f, this.fontSize);

			Immediate.Color(Themes.TextDisabled);
			UI.font.Write(this.hint, this.bounds.x0 + 0.32f, this.bounds.y0 + 0.06f, this.fontSize);
		}
		else {
			Immediate.Color(Themes.TextDisabled);
			UI.font.Write(this.hint, this.bounds.x0 + 0.01f, this.bounds.y0 + 0.06f, this.fontSize);
		}

		float left = RuntimeInformation.IsOSPlatform(OSPlatform.Windows) ? 0.21f : 0.14f;

		if (UI.TextButton("Open", new Rect(this.bounds.x1 - 0.11f, this.bounds.y0 + 0.06f, this.bounds.x1 - 0.01f, this.bounds.y0 + 0.01f), new UI.TextButtonMods() { disabled = this.selected.Count == 0 && this.typeFilter == SelectionType.File })) {
			this.Accept();
		}

		string croppedPath = UI.font.CropText(this.currentPath, this.bounds.x1 - 0.14f - (this.bounds.x0 + left), this.fontSize, out _, true);

		Immediate.Color(Themes.Text);
		UI.font.Write(croppedPath, this.bounds.x0 + left, this.bounds.y1 - 0.06f, this.fontSize);

		if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows)) {
			string root = Path.GetPathRoot(this.currentPath) ?? "";
			if (UI.TextButton(root, Rect.FromSize(this.bounds.x0 + 0.13f, this.bounds.y1 - 0.11f, 0.05f, 0.05f))) {
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

		if (this.search != null) {
			if (UI.TextureButton(UVRect.FromSize(this.bounds.x1 - 0.06f, this.bounds.y1 - 0.17f, 0.05f, 0.05f).UV(0f, 0f, 0.25f, 0.25f))) {
				this.search = null;
				this.Refresh();
			}
			else {
				Immediate.Color(Themes.TextDisabled);
				UI.FillRect(this.bounds.x0 + 0.1f, this.bounds.y1 - 0.12f, this.bounds.x1 - 0.07f, this.bounds.y1 - 0.17f);

				Immediate.Color(Themes.TextHighlight);
				string cropText = UI.font.CropText(this.search, this.bounds.x1 - this.bounds.x0 - 0.16f, this.fontSize, out float margin, true);
				margin = cropText.Length != this.search.Length ? margin : 0f;
				UI.font.Write(cropText, margin + this.bounds.x0 + 0.1f, this.bounds.y1 - 0.12f, this.fontSize);

				if (this.frame % 60 < 30) {
					Immediate.Color(Themes.Text);
					float cursorX = margin + this.bounds.x0 + 0.1f + UI.font.Measure(cropText, 0.04f).x;
					UI.FillRect(cursorX, this.bounds.y1 - 0.12f, cursorX + 0.005f, this.bounds.y1 - 0.17f);
				}
			}
		}

		if (this.newDirectory != null) {
			if (UI.TextureButton(UVRect.FromSize(this.bounds.x1 - 0.06f, this.bounds.y1 - 0.17f, 0.05f, 0.05f).UV(0f, 0f, 0.25f, 0.25f))) {
				this.newDirectory = null;
			}
			else {
				Immediate.Color(Themes.TextDisabled);
				UI.FillRect(this.bounds.x0 + 0.1f, this.bounds.y1 - 0.12f, this.bounds.x1 - 0.07f, this.bounds.y1 - 0.17f);

				Immediate.Color(Themes.TextHighlight);
				string cropText = UI.font.CropText(this.newDirectory, this.bounds.x1 - this.bounds.x0 - 0.16f, this.fontSize, out float margin, true);
				margin = cropText.Length != this.newDirectory.Length ? margin : 0f;
				UI.font.Write(cropText, margin + this.bounds.x0 + 0.1f, this.bounds.y1 - 0.12f, this.fontSize);

				if (this.frame % 60 < 30) {
					Immediate.Color(Themes.Text);
					float cursorX = margin + this.bounds.x0 + 0.1f + UI.font.Measure(cropText, 0.04f).x;
					UI.FillRect(cursorX, this.bounds.y1 - 0.12f, cursorX + 0.005f, this.bounds.y1 - 0.17f);
				}

				Immediate.Color(Themes.TextDisabled);
				this.DrawIcon(5, this.bounds.y1 - 0.12f);
			}
		}


		float offsetY = this.bounds.CenterY;
		float scrollAreaY1 = this.bounds.y1 - ((this.search == null && this.newDirectory == null) ? 0.12f : 0.18f);
		float scrollAreaY0 = this.bounds.y0 + 0.07f;
		float y = scrollAreaY1 - this.scroll;

		float padding = 0.01f;
		int width = Program.window.FramebufferSize.X;
		int height = Program.window.FramebufferSize.Y;

		Program.gl.Enable(EnableCap.ScissorTest);
		Program.gl.Scissor(
			(int) (((this.bounds.x0 + padding) / Main.screenBounds.x + 1f) * 0.5f * width),
			(int) ((scrollAreaY0 / Main.screenBounds.y + 1f) * 0.5f * height),
			(uint) ((this.bounds.x1 - this.bounds.x0 - padding * 2f) / Main.screenBounds.x * 0.5f * width),
			(uint) ((scrollAreaY1 - scrollAreaY0) / Main.screenBounds.y * 0.5f * height)
		);

		bool IsInScrollView(float itemY) => itemY <= scrollAreaY1 + 0.01f && itemY - 0.06f >= scrollAreaY0 - 0.01f;
		bool refreshing = false;

		foreach (string path in this.directories) {
			Rect rect = new Rect(this.bounds.x0 + 0.1f, y, this.bounds.x1 - 0.1f, y - 0.06f);
			bool hover = rect.Inside(Mouse.X, Mouse.Y) && !this.awaitingDeleteConfirmation && IsInScrollView(y);

			Immediate.Color(hover ? Themes.TextHighlight : Themes.Text);
			UI.font.Write(path + "/", this.bounds.x0 + 0.1f, y, this.fontSize);
			string currentFolderPath = Path.Join(this.currentPath, path);

			if (this.createdFolders.Contains(currentFolderPath)) {
				if (UI.TextureButton(new UVRect(this.bounds.x1 - 0.09f, y - 0.05f, this.bounds.x1 - 0.04f, y).UV(0f, 0f, 0.25f, 0.25f)) && hover) {
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
				this.search = null;
				this.newDirectory = null;
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

			Rect rect = new Rect(this.bounds.x0 + 0.1f, y, this.bounds.x1 - 0.1f, y - 0.06f);
			bool hover = rect.Inside(Mouse.X, Mouse.Y) && IsInScrollView(y);

			if (hover && Mouse.JustLeft) {
				if (this.allowMultiple && (Keys.Modifier(Keys.Modifiers.Shift) || Keys.Modifier(Keys.Modifiers.Control))) {
					if (Keys.Modifier(Keys.Modifiers.Shift)) {
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
					else if (Keys.Modifier(Keys.Modifiers.Control)) {
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

		if (this.noDirectoryPerms == true) {
			Immediate.Color(Themes.TextError);
			UI.font.Write("You do not have the required", this.bounds.CenterX, this.bounds.CenterY + 0.02f, 0.03f, Font.Align.MiddleCenter);
			Immediate.Color(Themes.TextError);
			UI.font.Write("permissions to access this folder!", this.bounds.CenterX, this.bounds.CenterY - 0.02f, 0.03f, Font.Align.MiddleCenter);
			Immediate.Color(Themes.TextError);
		}

		Program.gl.Disable(EnableCap.ScissorTest);
	}

	public enum SelectionType {
		File,
		Folder
	}
}
