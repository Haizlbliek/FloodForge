namespace FloodForge.Popups;

public abstract class Popup {
	protected bool hovered = false;
	protected bool minimized = false;
	protected bool slatedForDeletion = false;
	protected Vector2 vel;
	protected Rect bounds;

	public Popup() {
		this.bounds = new Rect(-0.5f, -0.5f, 0.5f, 0.5f);
	}

	public virtual void Draw() {
		if (Main.AprilFools) {
			this.bounds += this.vel * 0.2f;
			this.vel *= 0.95f;
			float bounce = 0.8f;
			if (this.bounds.x0 < -Main.screenBounds.x) {
				this.bounds += new Vector2(-Main.screenBounds.x - this.bounds.x0, 0f);
				this.vel.x = Math.Abs(this.vel.x) * bounce;
				Sfx.Play($"assets/objects/bump{Random.Shared.Next(1, 6)}.wav");
			}
			if (this.bounds.x1 > Main.screenBounds.x) {
				this.bounds += new Vector2(Main.screenBounds.x - this.bounds.x1, 0f);
				this.vel.x = -Math.Abs(this.vel.x) * bounce;
				Sfx.Play($"assets/objects/bump{Random.Shared.Next(1, 6)}.wav");
			}
			if (this.bounds.y1 > Main.screenBounds.y) {
				this.bounds += new Vector2(0f, Main.screenBounds.y - this.bounds.y1);
				this.vel.y = -Math.Abs(this.vel.y) * bounce;
				Sfx.Play($"assets/objects/bump{Random.Shared.Next(1, 6)}.wav");
			}
			if (this.bounds.y0 < -Main.screenBounds.y) {
				this.bounds += new Vector2(0f, -Main.screenBounds.y - this.bounds.y0);
				this.vel.y = Math.Abs(this.vel.y) * bounce;
				Sfx.Play($"assets/objects/bump{Random.Shared.Next(1, 6)}.wav");
			}
		}
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
			if (Main.AprilFools) {
				if (this.minimized) {
					Sfx.Play($"assets/objects/min.wav");
				}
				else {
					Sfx.Play($"assets/objects/max.wav");
				}
			}
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
		if (Main.AprilFools) Sfx.Play($"assets/objects/close.wav");
	}

	public virtual void Accept() => this.Close();
	public virtual void Reject() => this.Close();

	public virtual bool CanDrag(float mouseX, float mouseY) {
		if (mouseX >= this.bounds.x1 - 0.1f && mouseY >= this.bounds.y1 - 0.05f) return false;

		return mouseY >= this.bounds.y1 - 0.05f;
	}

	public void Offset(Vector2 offset) {
		if (Main.AprilFools) this.vel += offset;
		else this.bounds += offset;
	}
}