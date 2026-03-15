using FloodForge.Popups;

namespace FloodForge.Droplet;

public class ResizeLevelPopup : Popup {
	protected UI.TextInputEditable Width = new UI.TextInputEditable(UI.TextInputEditable.Type.UnsignedInteger, "48");
	protected UI.TextInputEditable Height = new UI.TextInputEditable(UI.TextInputEditable.Type.UnsignedInteger, "35");
	protected UI.TextInputEditable ScreenWidth = new UI.TextInputEditable(UI.TextInputEditable.Type.UnsignedFloat, "1.000", 3);
	protected UI.TextInputEditable ScreenHeight = new UI.TextInputEditable(UI.TextInputEditable.Type.UnsignedFloat, "1.000", 3);
	protected Vector2i resizeAnchor = new Vector2i(0, 0);
	protected bool stretchRoom = false;

	public ResizeLevelPopup() {
		this.bounds = new Rect(-0.5f, -0.21f, 0.5f, 0.21f);
		DropletWindow.resizeOffset = new Vector2i(0, 0);
		DropletWindow.resizeSize = new Vector2i(DropletWindow.Room.width, DropletWindow.Room.height);
		this.Width.SetValue(DropletWindow.Room.width);
		this.Height.SetValue(DropletWindow.Room.height);
		this.ScreenWidth.SetValue((DropletWindow.Room.width + 4) / 52f);
		this.ScreenHeight.SetValue((DropletWindow.Room.height + 5) / 40f);
	}

	protected void SetResizeOffset(Vector2i anchor, bool stretch) {
		if (stretch) {
			DropletWindow.resizeOffset = Vector2i.Zero;
		} else {
			DropletWindow.resizeOffset = new Vector2i(
				(int) ((DropletWindow.Room.width - DropletWindow.resizeSize.x) * (anchor.x + 1f) / 2f),
				(int) ((DropletWindow.Room.height - DropletWindow.resizeSize.y) * (anchor.y + 1f) / 2f)
			);
		}
	}

	public override void Draw() {
		base.Draw();

		if (this.minimized) return;

		try {
			DropletWindow.showResize = !this.slatedForDeletion;

			if (this.slatedForDeletion) {
				this.Width.Submit();
				this.Height.Submit();
				this.ScreenWidth.Submit();
				this.ScreenHeight.Submit();
			}

			float y = this.bounds.y1 - 0.05f;

			y -= 0.06f;
			Immediate.Color(Themes.Text);
			UI.font.Write("---- Room Size ----", this.bounds.x0 + 0.01f, y + 0.025f, 0.03f, Font.Center.Y);

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
			UI.font.Write("Width (Tiles)", this.bounds.x0 + 0.38f, y + 0.025f, 0.03f, Font.Center.Y);

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
			UI.font.Write("Height (Tiles)", this.bounds.x0 + 0.38f, y + 0.025f, 0.03f, Font.Center.Y);

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
			UI.font.Write("Width (Screens)", this.bounds.x0 + 0.38f, y + 0.025f, 0.03f, Font.Center.Y);

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
			UI.font.Write("Height (Screens)", this.bounds.x0 + 0.38f, y + 0.025f, 0.03f, Font.Center.Y);

			if (widthResponse.submitted) {
				DropletWindow.resizeSize.x = int.Parse(this.Width.value);
				this.ScreenWidth.SetValue((DropletWindow.resizeSize.x + 4) / 52.0f);
				this.SetResizeOffset(this.resizeAnchor, this.stretchRoom);
			}

			if (heightResponse.submitted) {
				DropletWindow.resizeSize.y = int.Parse(this.Height.value);
				this.ScreenHeight.SetValue((DropletWindow.resizeSize.y + 5) / 40.0f);
				this.SetResizeOffset(this.resizeAnchor, this.stretchRoom);
			}

			if (screenWidthResponse.submitted) {
				DropletWindow.resizeSize.x = (int) (float.Parse(this.ScreenWidth.value) * 52f - 4f);
				this.Width.SetValue(DropletWindow.resizeSize.x);
				this.SetResizeOffset(this.resizeAnchor, this.stretchRoom);
			}

			if (screenHeightResponse.submitted) {
				DropletWindow.resizeSize.y = (int) (float.Parse(this.ScreenHeight.value) * 40f - 5f);
				this.Height.SetValue(DropletWindow.resizeSize.y);
				this.SetResizeOffset(this.resizeAnchor, this.stretchRoom);
			}

			if (UI.TextureButton(UVRect.FromSize(this.bounds.x1 - 0.05f, this.bounds.y1 - 0.11f, 0.04f, 0.04f).UV(0.5f, 0f, 0.75f, 0.25f))) {
				this.Width.SetValue(DropletWindow.Room.width);
				this.Height.SetValue(DropletWindow.Room.height);
				this.ScreenWidth.SetValue((DropletWindow.Room.width + 4) / 52f);
				this.ScreenHeight.SetValue((DropletWindow.Room.height + 5) / 40f);
				this.resizeAnchor = Vector2i.Zero;
				this.stretchRoom = false;
			}

			for (int x = -1; x <= 1; x++) {
				for (int yp = -1; yp <= 1; yp++) {
					UVRect rect = UVRect.FromSize(this.bounds.x1 - 0.17f + x * 0.08f, this.bounds.y1 - 0.27f - yp * 0.08f, 0.07f, 0.07f);
					int diff = Math.Abs(x - this.resizeAnchor.x) + Math.Abs(yp - this.resizeAnchor.y);
					if (diff >= 2 || this.stretchRoom) {
						rect.UV(0.0f, 0.0f, 0.0f, 0.0f);
					} else if (diff == 0) {
						rect.UV(0.5f, 0.25f, 0.75f, 0.5f);
					} else if (x < this.resizeAnchor.x) {
						rect.UV(0.5f, 0.5f, 0.75f, 0.75f);
					} else if (x > this.resizeAnchor.x) {
						rect.UV(0.75f, 0.5f, 1.0f, 0.75f);
					} else if (yp < this.resizeAnchor.y) {
						rect.UV(0.75f, 0.75f, 1.0f, 1.0f);
					} else if (yp > this.resizeAnchor.y) {
						rect.UV(0.5f, 0.75f, 0.75f, 1.0f);
					}
					if (UI.TextureButton(rect, new UI.TextureButtonMods() { selected = x == this.resizeAnchor.x && yp == this.resizeAnchor.y, disabled = this.stretchRoom })) {
						this.resizeAnchor.x = x;
						this.resizeAnchor.y = yp;
						this.SetResizeOffset(this.resizeAnchor, this.stretchRoom);
					}
				}
			}

			Immediate.Color(Themes.Text);
			UI.font.Write("Stretch room", this.bounds.x1 - 0.06f, this.bounds.y1 - 0.4f, -0.03f, Font.Center.XY); // TODO: Right bottom align
			if (UI.CheckBox(Rect.FromSize(this.bounds.x1 - 0.06f, this.bounds.y1 - 0.41f, 0.05f, 0.05f), ref this.stretchRoom)) {
				DropletWindow.resizeSize = new Vector2i(int.Parse(this.Width.value), int.Parse(this.Height.value));
				this.SetResizeOffset(this.resizeAnchor, this.stretchRoom);
			}

			UI.ButtonResponse response = UI.TextButton("Resize room", Rect.FromSize(this.bounds.x0 + 0.01f, this.bounds.y1 - 0.41f, 0.3f, 0.05f), new UI.TextButtonMods { disabled = widthResponse.focused || heightResponse.focused || screenWidthResponse.focused || screenHeightResponse.focused });
			if (response.clicked) {
				DropletWindow.ResizeRoom(int.Parse(this.Width.value), int.Parse(this.Height.value), this.stretchRoom);
				this.Close();
			}

			Immediate.Color(Themes.Text);
			UI.font.Write("(No undo!)", bounds.x0 + 0.32f, bounds.y1 - 0.4f, 0.03f, Font.Center.Y); // TODO: Bottom align
		}
		catch (Exception) {
			Immediate.Color(Themes.Popup);
			UI.FillRect(this.bounds);
			Immediate.Color(Themes.Text);
			UI.font.Write("Error happened, sorry", this.bounds.CenterX, this.bounds.CenterY, 0.08f, Font.Center.XY);
			Logger.Info("An error happened when drawing resize level popup");
		}
	}

	public override void Close() {
		base.Close();

		DropletWindow.showResize = false;
	}
}