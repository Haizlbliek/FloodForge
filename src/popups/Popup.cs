namespace FloodForge.Popups;

public abstract class Popup {
	protected bool hovered = false;
	protected bool minimized = false;
	protected bool slatedForDeletion = false;
	protected Rect bounds;

	public Popup() {
		this.bounds = new Rect(-0.5f, -0.5f, 0.5f, 0.5f);
	}

	public virtual void Draw() {
		this.hovered = this.InteractBounds().Inside(Mouse.X, Mouse.Y);

		if (!this.minimized) {
			Immediate.Color(Themes.Popup);
			UI.FillRect(this.bounds.x0, this.bounds.y0, this.bounds.x1, this.bounds.y1);
		}

		Immediate.Color(Themes.PopupHeader);
		UI.FillRect(this.bounds.x0, this.bounds.y1 - 0.05f, this.bounds.x1, this.bounds.y1);

		if (UI.TextureButton(new UVRect(this.bounds.x1 - 0.05f, this.bounds.y1 - 0.05f, this.bounds.x1, this.bounds.y1).UV(0f, 0f, 0.25f, 0.25f))) {
			this.Close();
		}

		UVRect minimizedUv = new UVRect(this.bounds.x1 - 0.1f, this.bounds.y1 - 0.05f, this.bounds.x1 - 0.05f, this.bounds.y1);
		if (this.minimized) {
			minimizedUv.UV(0.25f, 0.5f, 0.5f, 0.75f);
		} else {
			minimizedUv.UV(0f, 0.5f, 0.25f, 0.75f);
		}

		if (UI.TextureButton(minimizedUv)) {
			this.minimized = !this.minimized;
		}

		Immediate.Color(this.hovered ? Themes.BorderHighlight : Themes.Border);
		if (this.minimized) {
			UI.StrokeRect(this.bounds.x0, this.bounds.y1 - 0.05f, this.bounds.x1, this.bounds.y1);
		} else {
			UI.StrokeRect(this.bounds.x0, this.bounds.y0, this.bounds.x1, this.bounds.y1);
		}
	}

	public virtual Rect InteractBounds() => this.minimized ? new Rect(this.bounds.x0, this.bounds.y1 - 0.05f, this.bounds.x1, this.bounds.y1) : this.bounds;

	public virtual void Close() {
		PopupManager.Remove(this);
		this.slatedForDeletion = true;
	}

	public virtual void Accept() => this.Close();
	public virtual void Reject() => this.Close();

	public virtual bool CanDrag(float mouseX, float mouseY) {
		if (mouseX >= this.bounds.x1 - 0.1f && mouseY >= this.bounds.y1 - 0.05f) return false;

		return mouseY >= this.bounds.y1 - 0.05f;
	}
	public void Offset(Vector2 offset) {
		this.bounds += offset;
	}
}