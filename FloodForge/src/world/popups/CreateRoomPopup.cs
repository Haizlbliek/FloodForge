using FloodForge.Droplet;
using FloodForge.Popups;
using Stride.Core.Extensions;

namespace FloodForge.World;

public class CreateRoomPopup : Popup {
	protected bool init = true;
	protected UI.TextInputEditable RoomName = new UI.TextInputEditable(UI.TextInputEditable.Type.Text, "");
	protected UI.TextInputEditable Width = new UI.TextInputEditable(UI.TextInputEditable.Type.UnsignedInteger, "48");
	protected UI.TextInputEditable Height = new UI.TextInputEditable(UI.TextInputEditable.Type.UnsignedInteger, "35");
	protected UI.TextInputEditable ScreenWidth = new UI.TextInputEditable(UI.TextInputEditable.Type.UnsignedFloat, "1.000", 3);
	protected UI.TextInputEditable ScreenHeight = new UI.TextInputEditable(UI.TextInputEditable.Type.UnsignedFloat, "1.000", 3);
	protected bool fillLayer1 = true;
	protected bool fillLayer2 = true;
	protected bool placeCameras = true;
	protected string errorText = "";

	protected static string GenerateRoomName(int screens) {
		string prefix = "";
		for (screens--; screens >= 0; screens = (screens / 26) - 1) {
			prefix = (char)('A' + (screens % 26)) + prefix;
		}

		int i = 1;
		while (PathUtil.FindFile(WorldWindow.region.roomsPath, $"{WorldWindow.region.acronym}_{prefix}{i:D2}.txt") != null) {
			i++;
		}

		return $"{prefix}{i:D2}";
	}

	public CreateRoomPopup() {
		WorldWindow.roomPlacementVisualiser.size = new Vector2i(48, 35);
	}

	public override void Draw() {
		base.Draw();
		if (this.collapsed) return;

		float y = this.bounds.y1 - 0.05f;

		y -= 0.06f;
		Immediate.Color(Themes.Text);
		UI.font.Write("---- Room Name ----", this.bounds.x0 + 0.01f, y + 0.025f, 0.03f, Font.Align.MiddleLeft);

		y -= 0.06f;
		Immediate.Color(Themes.Text);
		UI.font.Write($"{WorldWindow.region.acronym}_", this.bounds.x0 + 0.01f, y + 0.025f, 0.03f, Font.Align.MiddleLeft);
		float roomNameX = UI.font.Measure($"{WorldWindow.region.acronym}_", 0.03f).x;
		UI.TextInputResponse roomNameResponse = UI.TextInput(Rect.FromSize(this.bounds.x0 + 0.01f + roomNameX, y, 0.35f, 0.05f), this.RoomName);
		if (UI.TextButton("Generate", Rect.FromSize(this.bounds.x0 + 0.01f + roomNameX + 0.36f, y, 0.2f, 0.05f), new UI.TextButtonMods { disabled = this.ScreenWidth.Focused || this.ScreenHeight.Focused || this.RoomName.Focused })) {
			try {
				int screens = Mathf.RoundToInt(MathF.Max(float.Parse(this.ScreenWidth.value), 1f) * MathF.Max(float.Parse(this.ScreenHeight.value), 1f));
				this.RoomName.value = GenerateRoomName(screens);
				this.RoomName.submitted = true;
			}
			catch (Exception) {}
		}

		y -= 0.06f;
		Immediate.Color(Themes.Text);
		UI.font.Write("---- Room Size ----", this.bounds.x0 + 0.01f, y + 0.025f, 0.03f, Font.Align.MiddleLeft);

		y -= 0.06f;
		UI.TextInputResponse widthResponse = UI.TextInput(Rect.FromSize(this.bounds.x0 + 0.01f, y, 0.25f, 0.05f), this.Width);
		if (UI.TextureButton(UVRect.FromSize(this.bounds.x0 + 0.27f, y, 0.05f, 0.05f).UV(0.0f, 0.5f, 0.25f, 0.75f), new UI.TextureButtonMods { disabled = widthResponse.focused })) {
			this.Width.value = Math.Max(int.Parse(this.Width.value) - 1, 1).ToString();
			widthResponse = new UI.TextInputResponse(widthResponse.focused, widthResponse.hovered, true);
		}
		if (UI.TextureButton(UVRect.FromSize(this.bounds.x0 + 0.32f, y, 0.05f, 0.05f).UV(0.25f, 0.5f, 0.5f, 0.75f), new UI.TextureButtonMods { disabled = widthResponse.focused })) {
			this.Width.value = Math.Max(int.Parse(this.Width.value) + 1, 1).ToString();
			widthResponse = new UI.TextInputResponse(widthResponse.focused, widthResponse.hovered, true);
		}
		Immediate.Color(Themes.Text);
		UI.font.Write("Width (Tiles)", this.bounds.x0 + 0.38f, y + 0.025f, 0.03f, Font.Align.MiddleLeft);

		y -= 0.06f;
		UI.TextInputResponse heightResponse = UI.TextInput(Rect.FromSize(this.bounds.x0 + 0.01f, y, 0.25f, 0.05f), this.Height);
		if (UI.TextureButton(UVRect.FromSize(this.bounds.x0 + 0.27f, y, 0.05f, 0.05f).UV(0.0f, 0.5f, 0.25f, 0.75f), new UI.TextureButtonMods { disabled = heightResponse.focused })) {
			this.Height.value = Math.Max(int.Parse(this.Height.value) - 1, 1).ToString();
			heightResponse = new UI.TextInputResponse(heightResponse.focused, heightResponse.hovered, true);
		}
		if (UI.TextureButton(UVRect.FromSize(this.bounds.x0 + 0.32f, y, 0.05f, 0.05f).UV(0.25f, 0.5f, 0.5f, 0.75f), new UI.TextureButtonMods { disabled = heightResponse.focused })) {
			this.Height.value = Math.Max(int.Parse(this.Height.value) + 1, 1).ToString();
			heightResponse = new UI.TextInputResponse(heightResponse.focused, heightResponse.hovered, true);
		}
		Immediate.Color(Themes.Text);
		UI.font.Write("Height (Tiles)", this.bounds.x0 + 0.38f, y + 0.025f, 0.03f, Font.Align.MiddleLeft);

		y -= 0.06f;
		UI.TextInputResponse screenWidthResponse = UI.TextInput(Rect.FromSize(this.bounds.x0 + 0.01f, y, 0.25f, 0.05f), this.ScreenWidth);
		if (UI.TextureButton(UVRect.FromSize(this.bounds.x0 + 0.27f, y, 0.05f, 0.05f).UV(0.0f, 0.5f, 0.25f, 0.75f), new UI.TextureButtonMods { disabled = screenWidthResponse.focused })) {
			this.ScreenWidth.value = (float.Parse(this.ScreenWidth.value) - 0.5f).ToString($"F{this.ScreenWidth.floatDecimalCount}");
			screenWidthResponse = new UI.TextInputResponse(screenWidthResponse.focused, screenWidthResponse.hovered, true);
		}
		if (UI.TextureButton(UVRect.FromSize(this.bounds.x0 + 0.32f, y, 0.05f, 0.05f).UV(0.25f, 0.5f, 0.5f, 0.75f), new UI.TextureButtonMods { disabled = screenWidthResponse.focused })) {
			this.ScreenWidth.value = (float.Parse(this.ScreenWidth.value) + 0.5f).ToString($"F{this.ScreenWidth.floatDecimalCount}");
			screenWidthResponse = new UI.TextInputResponse(screenWidthResponse.focused, screenWidthResponse.hovered, true);
		}
		Immediate.Color(Themes.Text);
		UI.font.Write("Width (Screens)", this.bounds.x0 + 0.38f, y + 0.025f, 0.03f, Font.Align.MiddleLeft);

		y -= 0.06f;
		UI.TextInputResponse screenHeightResponse = UI.TextInput(Rect.FromSize(this.bounds.x0 + 0.01f, y, 0.25f, 0.05f), this.ScreenHeight);
		if (UI.TextureButton(UVRect.FromSize(this.bounds.x0 + 0.27f, y, 0.05f, 0.05f).UV(0.0f, 0.5f, 0.25f, 0.75f), new UI.TextureButtonMods { disabled = screenHeightResponse.focused })) {
			this.ScreenHeight.value = (float.Parse(this.ScreenHeight.value) - 0.5f).ToString($"F{this.ScreenHeight.floatDecimalCount}");
			screenHeightResponse = new UI.TextInputResponse(screenHeightResponse.focused, screenHeightResponse.hovered, true);
		}
		if (UI.TextureButton(UVRect.FromSize(this.bounds.x0 + 0.32f, y, 0.05f, 0.05f).UV(0.25f, 0.5f, 0.5f, 0.75f), new UI.TextureButtonMods { disabled = screenHeightResponse.focused })) {
			this.ScreenHeight.value = (float.Parse(this.ScreenHeight.value) + 0.5f).ToString($"F{this.ScreenHeight.floatDecimalCount}");
			screenHeightResponse = new UI.TextInputResponse(screenHeightResponse.focused, screenHeightResponse.hovered, true);
		}
		Immediate.Color(Themes.Text);
		UI.font.Write("Height (Screens)", this.bounds.x0 + 0.38f, y + 0.025f, 0.03f, Font.Align.MiddleLeft);


		y -= 0.06f;
		Immediate.Color(Themes.Text);
		UI.font.Write("---- Fill Layers ----", this.bounds.x0 + 0.01f, y + 0.025f, 0.03f, Font.Align.MiddleLeft);

		y -= 0.06f;
		if (UI.TextButton("1", Rect.FromSize(this.bounds.x0 + 0.01f, y, 0.05f, 0.05f), new UI.TextButtonMods { selected = this.fillLayer1 })) {
			this.fillLayer1 = !this.fillLayer1;
		}
		if (UI.TextButton("2", Rect.FromSize(this.bounds.x0 + 0.07f, y, 0.05f, 0.05f), new UI.TextButtonMods { selected = this.fillLayer2 })) {
			this.fillLayer2 = !this.fillLayer2;
		}


		y -= 0.06f;
		Immediate.Color(Themes.Text);
		UI.font.Write("---- Options ----", this.bounds.x0 + 0.01f, y + 0.025f, 0.03f, Font.Align.MiddleLeft);

		y -= 0.06f;
		Immediate.Color(Themes.Text);
		UI.font.Write("Auto-place Cameras", this.bounds.x0 + 0.07f, y + 0.025f, 0.03f, Font.Align.MiddleLeft);
		UI.CheckBox(Rect.FromSize(this.bounds.x0 + 0.01f, y, 0.05f, 0.05f), ref this.placeCameras);


		if (widthResponse.submitted) {
			WorldWindow.roomPlacementVisualiser.size.x = int.Parse(this.Width.value);
			this.ScreenWidth.value = ((WorldWindow.roomPlacementVisualiser.size.x + 4) / 52.0f).ToString($"F{this.ScreenWidth.floatDecimalCount}");
		}

		if (heightResponse.submitted) {
			WorldWindow.roomPlacementVisualiser.size.y = int.Parse(this.Height.value);
			this.ScreenHeight.value = ((WorldWindow.roomPlacementVisualiser.size.y + 5) / 40.0f).ToString($"F{this.ScreenHeight.floatDecimalCount}");
		}

		if (screenWidthResponse.submitted) {
			WorldWindow.roomPlacementVisualiser.size.x = (int) (float.Parse(this.ScreenWidth.value) * 52f - 4f);
			this.Width.value = WorldWindow.roomPlacementVisualiser.size.x.ToString();
		}

		if (screenHeightResponse.submitted) {
			WorldWindow.roomPlacementVisualiser.size.y = (int) (float.Parse(this.ScreenHeight.value) * 40f - 5f);
			this.Height.value = WorldWindow.roomPlacementVisualiser.size.y.ToString();
		}

		if (UI.TextureButton(UVRect.FromSize(this.bounds.x1 - 0.05f, this.bounds.y1 - 0.11f, 0.05f, 0.05f).UV(0.5f, 0.05f, 0.75f, 0.25f))) {
			this.RoomName.value = "";
			this.Width.value = "48";
			this.Height.value = "35";
			this.ScreenWidth.value = "1.000";
			this.ScreenHeight.value = "1.000";
			this.fillLayer1 = true;
			this.fillLayer2 = true;
			WorldWindow.roomPlacementVisualiser.size = new Vector2i(48, 35);
		}

		bool canCreate = true;
		if (this.RoomName.value.IsNullOrEmpty()) canCreate = false;
		if (widthResponse.focused) canCreate = false;
		if (!int.TryParse(this.Width.value, out int wVal) || wVal <= 0) canCreate = false;
		if (heightResponse.focused) canCreate = false;
		if (!int.TryParse(this.Height.value, out int hVal) || hVal <= 0) canCreate = false;
		if (screenWidthResponse.focused) canCreate = false;
		if (screenHeightResponse.focused) canCreate = false;

		if (roomNameResponse.focused) {
			this.errorText = "";
			canCreate = false;
		}
		else if (roomNameResponse.submitted) {
			if (this.RoomName.value.IsNullOrEmpty()) {
				this.errorText = "";
				canCreate = false;
			}
			else {
				string name = $"{WorldWindow.region.acronym}_{this.RoomName.value}";
				if (PathUtil.FindFile(WorldWindow.region.roomsPath, name + ".txt") != null) {
					canCreate = false;
					this.errorText = "Room with that name already exists in files!";
				}
			}
		}

		if (!this.errorText.IsNullOrEmpty()) {
			canCreate = false;
			Immediate.Color(Themes.Text);
			UI.font.Write(this.errorText, this.bounds.x0 + 0.01f, this.bounds.y0 + 0.035f, 0.03f, Font.Align.MiddleLeft);
		}

		if (UI.TextButton("Create", new Rect(this.bounds.x0 + 0.01f, this.bounds.y0 + 0.01f, this.bounds.x0 + 0.26f, this.bounds.y0 + 0.06f), new UI.TextButtonMods { disabled = !canCreate })) {
			string name = $"{WorldWindow.region.acronym}_{this.RoomName.value}";
			Droplet.LevelUtils.CreateLevelFiles(WorldWindow.region.roomsPath, name, int.Parse(this.Width.value), int.Parse(this.Height.value), this.fillLayer1, this.fillLayer2, this.placeCameras);
			this.Close();

			Room room = new Room(Path.Join(WorldWindow.region.roomsPath, $"{name}.txt"), name) {
				CanonPosition = WorldWindow.roomPlacementVisualiser.Position,
				DevPosition = WorldWindow.roomPlacementVisualiser.Position
			};
			WorldWindow.region.rooms.Add(room);
			Main.mode = Main.Mode.Droplet;
			DropletWindow.LoadRoom(room);
		}
	}

	public override void Close() {
		base.Close();
		WorldWindow.placingRoom = false;
	}
}