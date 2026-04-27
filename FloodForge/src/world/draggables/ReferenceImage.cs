namespace FloodForge.World;

public class ReferenceImage : WorldDraggable {
	public string imagePath;
	public Texture image;
	public float Height => this.image.height * this.scale;
	public float Width => this.image.width * this.scale;
	public float brightness = 1f;
	protected float scale;
	public float Scale {
		get {
			return this.scale;
		}
		set {
			this.scale = value;
			this.UpdateBounds();
		}
	}
	public override void SetPosition(Vector2 value) {
		base.SetPosition(value);
		this.UpdateBounds();
	}
	public bool lockImage = false;
	public bool drawUnderGrid = true;
	public override bool IsDraggable() {
		return this.Visible & !this.lockImage;
	}
	public Rect imageBounds;

	public ReferenceImage(string path) {
		if (!Path.Exists(path)) {
			throw new FileNotFoundException("Invalid reference image path!");
		}
		this.imagePath = path;
		this.image = Texture.Load(path, TextureWrapMode.ClampToBorder);
		this.Scale = 300f / this.image.width;
	}

	public void UpdateBounds() {
		this.imageBounds = new Rect(this.Position.x - this.Width, this.Position.y + this.Height, this.position.x + this.Width, this.Position.y - this.Height);
	}

	public void Draw() {
		if (this.Visible) {
			UI.CenteredTexture(this.image, this.Position.x, this.Position.y, this.Width * 2);

			if (WorldWindow.selectedDraggables.Contains(this)) {
				Immediate.Color(Themes.RoomBorderHighlight);
				UI.StrokeRect(this.imageBounds);
			}
			if(this.brightness != 1f) {
				Program.gl.Enable(EnableCap.Blend);
				Immediate.Color(Themes.Background);
				Immediate.Alpha(1f - this.brightness);
				UI.FillRect(this.imageBounds);
				Program.gl.Disable(EnableCap.Blend);
				Immediate.Alpha(1f);
			}
		}
	}

	public bool Inside(Vector2 pos) {
		return pos.x >= this.imageBounds.x0 && pos.y >= this.imageBounds.y0 && pos.x < this.imageBounds.x1 && pos.y <= this.imageBounds.y1;
	}

	public bool Intersects(Vector2 from, Vector2 to) {
		Vector2 cornerMin = Vector2.Min(from, to);
		Vector2 cornerMax = Vector2.Max(from, to);

		return cornerMax.x >=  this.imageBounds.x0 && cornerMax.y >= this.imageBounds.y0 && cornerMin.x < this.imageBounds.x1 && cornerMin.y <= this.imageBounds.y1;
	}
}