using FloodForge.Droplet;

namespace FloodForge.Popups;

public class SelectionButtonPopup : Popup {
	protected override bool Resizable => false;
	protected  UVRect clearButton;
	protected  UVRect copyButton;
	protected  UVRect cutButton;
	protected  UVRect pasteButton;
	protected string hoverText = "";

	public SelectionButtonPopup() {
		this.SetSize(new (0.34f, 0.07f));
	}

	public void SetPosition(Vector2 position) {
		this.bounds = new Rect(position.x, position.y, position.x + 0.3f, position.y + 0.07f);
	}

	public Vector2 GetBottomLeft() {
		return new Vector2(this.bounds.x0, this.bounds.y1);
	}
	
	public override void Draw() {
		if (DropletWindow.selectionState < 2) {
			this.Close();
			return;
		}

		this.hoverText = "";

		this.clearButton = new UVRect(this.bounds.x0 + 0.01f, this.bounds.y0 + 0.01f, this.bounds.x0 + 0.06f, this.bounds.y1 - 0.01f).UV(0.0f,0.25f,0.25f,0.0f);
		UI.ButtonResponse clearResponse = UI.TextureButton(this.clearButton);
		if (clearResponse.clicked) {
			this.Close();
		}
		if (clearResponse.hovered) {
			this.hoverText = "Clear Selection";
		}
		
		if (DropletWindow.selectionState == 2) {
			this.copyButton = new UVRect(this.bounds.x0 + 0.07f, this.bounds.y0 + 0.01f, this.bounds.x0 + 0.12f, this.bounds.y1 - 0.01f).UV(0.0f,0.5f,0.25f,0.25f);
			UI.ButtonResponse copyResponse = UI.TextureButton(this.copyButton);
			if (copyResponse.clicked) {
				DropletWindow.selectionState = 3;
				DropletWindow.selectionModificationMode = 0;
			}
			if (copyResponse.hovered) {
				this.hoverText = "Copy Selection";
			}

			this.cutButton = new UVRect(this.bounds.x0 + 0.13f, this.bounds.y0 + 0.01f, this.bounds.x0 + 0.18f, this.bounds.y1 - 0.01f).UV(0.25f,0.25f,0.5f,0.0f);
			UI.ButtonResponse cutResponse = UI.TextureButton(this.cutButton);
			if (cutResponse.clicked) {
				DropletWindow.selectionState = 3;
				DropletWindow.selectionModificationMode = 1;
			}
			if (cutResponse.hovered) {
				this.hoverText = "Cut Selection";
			}
		}
		else if (DropletWindow.selectionState == 4) {
			this.pasteButton = new UVRect(this.bounds.x0 + 0.07f, this.bounds.y0 + 0.01f, this.bounds.x0 + 0.12f, this.bounds.y1 - 0.01f).UV(0.25f,0.0f,0.5f,0.25f);
			UI.ButtonResponse pasteResponse = UI.TextureButton(this.pasteButton);
			if (pasteResponse.clicked) {
				DropletWindow.selectionState = 5;
				this.Close();
			}
			if (pasteResponse.hovered) {
				this.hoverText = "Paste Selection";
			}
		}

		if (this.hoverText.Length != 0) {
			float width = UI.font.Measure(this.hoverText, 0.03f).x;
			Rect hoverRect = Rect.FromSize(Mouse.X, Mouse.Y, width + 0.02f, 0.05f);
			if (hoverRect.x1 > Main.screenBounds.x) {
				hoverRect += new Vector2(Main.screenBounds.x - hoverRect.x1, 0f);
			}
			if (hoverRect.y1 > Main.screenBounds.y) {
				hoverRect += new Vector2(0f, Main.screenBounds.y - hoverRect.y1);
			}

			Immediate.Color(Themes.Background);
			UI.FillRect(hoverRect);
			Immediate.Color(Themes.Text);
			UI.font.Write(this.hoverText, hoverRect.x0 + 0.01f, hoverRect.CenterY, 0.03f, Font.Align.MiddleLeft);
			Immediate.Color(Themes.Border);
			UI.StrokeRect(hoverRect);
		}
	}

	public override void Close() {
		DropletWindow.selectionState = -1;
		base.Close();
	}
}