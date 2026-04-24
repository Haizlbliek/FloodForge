using Silk.NET.Input;

namespace FloodForge.Popups;

public abstract class Popup {
	protected bool cursorOverButton = false;
	protected bool hovered = false;
	protected bool coyoteHover = false;
	protected bool minimized = false;
	protected bool slatedForDeletion = false;
	protected UVRect closeButton;
	protected UVRect minimizeButton;
	protected Rect bounds;
	protected Rect minimumBounds;
	protected Rect initBounds;
	protected bool initBoundsSet = false;
	protected bool resizeable = true;

	protected Rect scaleControlIncluder;
	protected Rect scaleControlExcluder;
	protected float scaleControlInnerMargin = 0.02f;
	protected float scaleControlOuterMargin = 0.02f;
	protected RectPosition mouseEdge;
	protected bool resizingWindow = false;
	protected Vector2 pivotPoint;
	protected Vector2i pivotDir;
	protected Vector2 currentMousePos;
	protected Rect newBounds;

	protected bool mouseCursorSet = false;
	protected bool hadMouseCursorSet = false;

	public bool Resizing => this.resizingWindow;

	public Popup() {
		this.bounds = new Rect(-0.5f, -0.5f, 0.5f, 0.5f);
		this.minimumBounds = new Rect(-0.1f, -0.1f, 0.1f, 0.1f);
		this.closeButton = new UVRect(this.bounds.x1 - 0.05f, this.bounds.y1 - 0.05f, this.bounds.x1, this.bounds.y1).UV(0f, 0f, 0.25f, 0.25f);
		this.minimizeButton = new UVRect(this.bounds.x1 - 0.1f, this.bounds.y1 - 0.05f, this.bounds.x1 - 0.05f, this.bounds.y1).UV(0f, 0.5f, 0.25f, 0.75f);
		this.UpdateScaleControls(this.bounds);
	}

	private void UpdateResizing() {
		this.UpdateScaleControls(this.bounds);
		this.hovered = this.InteractBounds().Inside(Mouse.X, Mouse.Y);
		if (this.hovered == true) {
			this.coyoteHover = true;
		}
		bool isOutsideExcludor = !this.scaleControlExcluder.Inside(Mouse.X, Mouse.Y);
		bool isInsideIncluder = this.scaleControlIncluder.Inside(Mouse.X, Mouse.Y);
		if (this.CanDrag(Mouse.X, Mouse.Y)) {
			return;
		}

		if (!this.minimized && !this.cursorOverButton && !this.resizingWindow) {
			Immediate.Color(Themes.Layer2Color);
			if (this.coyoteHover && isOutsideExcludor) {
				if (isInsideIncluder) {
					if (Settings.DEBUGVisiblePopupVisuals)
						UI.FillRect(this.scaleControlExcluder.x0, this.scaleControlExcluder.y0, this.scaleControlExcluder.x1, this.scaleControlExcluder.y1);

					for (int x = 0; x <= 2; x++) {
						for (int y = 0; y <= 2; y++) {
							if (x == 1 && y == 1)
								continue;

							float currX0 = x == 0 ? this.scaleControlIncluder.x0 : x == 1 ? this.scaleControlExcluder.x0 : this.scaleControlExcluder.x1;
							float currX1 = x == 0 ? this.scaleControlExcluder.x0 : x == 1 ? this.scaleControlExcluder.x1 : this.scaleControlIncluder.x1;
							float currY0 = y == 0 ? this.scaleControlIncluder.y0 : y == 1 ? this.scaleControlExcluder.y0 : this.scaleControlExcluder.y1;
							float currY1 = y == 0 ? this.scaleControlExcluder.y0 : y == 1 ? this.scaleControlExcluder.y1 : this.scaleControlIncluder.y1;
							Rect currRect = new Rect(currX0, currY0, currX1, currY1);
							if (currRect.Inside(Mouse.X, Mouse.Y)) {
								if (Settings.DEBUGVisiblePopupVisuals)
									UI.StrokeRect(currRect);

								this.mouseEdge = (RectPosition) (x + y * 3);
								switch (this.mouseEdge) {
									case RectPosition.Center:
										Main.mouse?.Cursor.StandardCursor = StandardCursor.ResizeAll;
										this.mouseCursorSet = true;
										break;
									case RectPosition.UpMid:
									case RectPosition.DownMid:
										Main.mouse?.Cursor.StandardCursor = StandardCursor.VResize;
										this.mouseCursorSet = true;
										break;
									case RectPosition.MidLeft:
									case RectPosition.MidRight:
										Main.mouse?.Cursor.StandardCursor = StandardCursor.HResize;
										this.mouseCursorSet = true;
										break;
									case RectPosition.UpLeft:
									case RectPosition.DownRight:
										Main.mouse?.Cursor.StandardCursor = StandardCursor.NeswResize;
										this.mouseCursorSet = true;
										break;
									case RectPosition.UpRight:
									case RectPosition.DownLeft:
										Main.mouse?.Cursor.StandardCursor = StandardCursor.NwseResize;
										this.mouseCursorSet = true;
										break;
								}

								if (Mouse.JustLeft) {
									this.resizingWindow = true;
									this.pivotPoint = this.mouseEdge switch {
										RectPosition.UpLeft => new(this.bounds.x1, this.bounds.y1),
										RectPosition.UpMid => new(this.bounds.CenterX, this.bounds.y1),
										RectPosition.UpRight => new(this.bounds.x0, this.bounds.y1),
										RectPosition.MidLeft => new(this.bounds.x1, this.bounds.CenterY),
										RectPosition.Center => new(this.bounds.CenterX, this.bounds.CenterY),
										RectPosition.MidRight => new(this.bounds.x0, this.bounds.CenterY),
										RectPosition.DownLeft => new(this.bounds.x1, this.bounds.y0),
										RectPosition.DownMid => new(this.bounds.CenterX, this.bounds.y0),
										RectPosition.DownRight => new(this.bounds.x0, this.bounds.y0),
										_ => new(0, 0)
									};
									this.pivotDir = new Vector2i(float.Sign((Mouse.Pos - this.pivotPoint).x), float.Sign((Mouse.Pos - this.pivotPoint).y));
								}
							}
						}
					}
				}
				else {
					this.coyoteHover = false;
				}
			}
			else {
				if (Settings.DEBUGVisiblePopupVisuals)
					UI.StrokeRect(this.scaleControlExcluder.x0, this.scaleControlExcluder.y0, this.scaleControlExcluder.x1, this.scaleControlExcluder.y1);
			}
			if (Mouse.JustRight) {
				if (this.initBoundsSet)
					this.ResizePopup(this.initBounds);
			}
		}
		else if (this.resizingWindow) {
			this.mouseCursorSet = true;
			if (!this.initBoundsSet) {
				this.initBounds = this.bounds;
				this.minimumBounds = this.bounds;
				this.initBoundsSet = true;
			}
			if (Mouse.JustRight) {
				this.resizingWindow = false;
				this.ResizePopup(this.initBounds);
			}
			if (!Mouse.Left) {
				this.resizingWindow = false;
				this.ResizePopup(this.newBounds);
			}
			else {
				this.currentMousePos = Mouse.Pos;
				Rect originalBounds = this.bounds;
				bool lockXScale = false;
				bool lockYScale = false;
				if (int.IsOddInteger((int) this.mouseEdge)) {
					if (this.mouseEdge == (RectPosition) 1 || this.mouseEdge == (RectPosition) 7) {
						lockXScale = true;
					}
					else {
						lockYScale = true;
					}
				}
				Vector2 PointB = new Vector2(
					this.currentMousePos.x,
					this.currentMousePos.y);

				// All of this can probably be improved in terms of legibility.
				// I feel like a lot of this is expecting cases that cannot exist.
				float newX0 = lockXScale || this.pivotDir.x > 0 ? originalBounds.x0 : Math.Min(PointB.x, this.pivotPoint.x);
				float newX1 = lockXScale || this.pivotDir.x < 0 ? originalBounds.x1 : Math.Max(PointB.x, this.pivotPoint.x);
				float newY0 = lockYScale || this.pivotDir.y > 0 ? originalBounds.y0 : Math.Min(PointB.y, this.pivotPoint.y);
				float newY1 = lockYScale || this.pivotDir.y < 0 ? originalBounds.y1 : Math.Max(PointB.y, this.pivotPoint.y);
				if (!lockXScale) {
					if (this.pivotDir.x > 0)
						newX1 = Math.Max(this.pivotPoint.x + (this.minimumBounds.x1 - this.minimumBounds.x0), newX1);
					else
						newX0 = Math.Min(this.pivotPoint.x - (this.minimumBounds.x1 - this.minimumBounds.x0), newX0);
				}
				if (!lockYScale) {
					if (this.pivotDir.y > 0)
						newY1 = Math.Max(this.pivotPoint.y + (this.minimumBounds.y1 - this.minimumBounds.y0), newY1);
					else
						newY0 = Math.Min(this.pivotPoint.y - (this.minimumBounds.y1 - this.minimumBounds.y0), newY0);
				}

				this.newBounds = new(newX0, newY0, newX1, newY1);
				if (Settings.DEBUGVisiblePopupVisuals) {
					Immediate.Color(Themes.BorderHighlight);
					UI.StrokeRect(originalBounds);
					UI.StrokeCircle(new(originalBounds.x0, originalBounds.y0), 0.02f, 8);
					Immediate.Color(Themes.Layer2Color);
					UI.StrokeCircle(PointB, 0.02f, 8);
					UI.StrokeCircle(this.pivotPoint + (Vector2) this.pivotDir * 0.1f, 0.02f, 8);
					Immediate.Color(Themes.Layer1Color);
					UI.StrokeCircle(this.currentMousePos, 0.02f, 8);
					Immediate.Color(Themes.Layer0Color);
					UI.StrokeCircle(this.pivotPoint, 0.02f, 8);
					Immediate.Color(Themes.RoomAir);
					UI.StrokeCircle(new(newX0, newY0), 0.02f, 8);
				}
				Immediate.Color(Themes.BorderHighlight);
				UI.StrokeRect(this.newBounds);
			}
		}
	}

	public virtual void Draw() {
		if (this.closeButton.Inside(Mouse.Pos) || this.minimizeButton.Inside(Mouse.Pos)) {
			this.cursorOverButton = true;
		}

		if (!this.minimized) {
			Immediate.Color(Themes.Popup);
			UI.ButtonFillRect(this.bounds.x0, this.bounds.y0, this.bounds.x1, this.bounds.y1);
		}

		Immediate.Color(Themes.PopupHeader);
		UI.ButtonFillRect(this.bounds.x0, this.bounds.y1 - 0.05f, this.bounds.x1, this.bounds.y1);

		this.closeButton = new UVRect(this.bounds.x1 - 0.05f, this.bounds.y1 - 0.05f, this.bounds.x1, this.bounds.y1).UV(0f, 0f, 0.25f, 0.25f);
		if (UI.TextureButton(this.closeButton)) {
			this.Close();
		}

		this.minimizeButton = new UVRect(this.bounds.x1 - 0.1f, this.bounds.y1 - 0.05f, this.bounds.x1 - 0.05f, this.bounds.y1);
		if (this.minimized) {
			this.minimizeButton.UV(0.25f, 0.5f, 0.5f, 0.75f);
		}
		else {
			this.minimizeButton.UV(0f, 0.5f, 0.25f, 0.75f);
		}

		if (UI.TextureButton(this.minimizeButton)) {
			this.minimized = !this.minimized;
		}

		Immediate.Color(this.hovered ? Themes.BorderHighlight : Themes.Border);
		if (this.minimized) {
			UI.ButtonStrokeRect(this.bounds.x0, this.bounds.y1 - 0.05f, this.bounds.x1, this.bounds.y1);
		}
		else {
			UI.ButtonStrokeRect(this.bounds.x0, this.bounds.y0, this.bounds.x1, this.bounds.y1);
		}

		if (this.CanDrag(Mouse.X, Mouse.Y)) {
			Main.mouse?.Cursor.StandardCursor = StandardCursor.ResizeAll;
			this.mouseCursorSet = true;
		}

		if (this.resizeable) {
			this.UpdateResizing();
		}
		else if (this.initBoundsSet) {
			this.ResizePopup(this.initBounds);
		}

		this.cursorOverButton = false;
	}

	public void FinishDraw() {
		if (!this.mouseCursorSet && this.hadMouseCursorSet) {
			Main.mouse?.Cursor.StandardCursor = StandardCursor.Default;
			this.hadMouseCursorSet = false;
		}
		this.hadMouseCursorSet = this.mouseCursorSet;
		this.mouseCursorSet = false;
	}

	public virtual Rect InteractBounds() => this.minimized ? new Rect(this.bounds.x0, this.bounds.y1 - 0.05f, this.bounds.x1, this.bounds.y1) : this.bounds;

	public virtual void UpdateScaleControls(Rect bounds) {
		this.scaleControlExcluder = new Rect(
			bounds.x0 + this.scaleControlInnerMargin,
			bounds.y0 + this.scaleControlInnerMargin,
			bounds.x1 - this.scaleControlInnerMargin,
			bounds.y1 - this.scaleControlInnerMargin
			);
		this.scaleControlIncluder = new Rect(
			bounds.x0 - this.scaleControlOuterMargin,
			bounds.y0 - this.scaleControlOuterMargin,
			bounds.x1 + this.scaleControlOuterMargin,
			bounds.y1 + this.scaleControlOuterMargin * 2
			);
	}

	public virtual void ResizePopup(Rect newRect) {
		this.bounds = newRect;
	}

	public virtual void Close() {
		if (this.mouseCursorSet || this.hadMouseCursorSet) {
			Main.mouse?.Cursor.StandardCursor = StandardCursor.Default;
			this.hadMouseCursorSet = false;
		}
		PopupManager.Remove(this);
		this.slatedForDeletion = true;
	}

	public virtual void Accept() => this.Close();
	public virtual void Reject() => this.Close();

	public virtual bool CanDrag(float mouseX, float mouseY) {
		if (this.resizingWindow)
			return false;
		if (mouseX <= this.bounds.x1 - 0.1f && mouseX >= this.bounds.x0 && mouseY <= this.bounds.y1 && mouseY >= this.bounds.y1 - 0.05f) return true;

		return false;
	}

	public void Offset(Vector2 offset) {
		this.bounds += offset;
	}

	public enum RectPosition {
		UpLeft,
		UpMid,
		UpRight,
		MidLeft,
		Center,
		MidRight,
		DownLeft,
		DownMid,
		DownRight
	}
}