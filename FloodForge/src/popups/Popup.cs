using Silk.NET.Input;

namespace FloodForge.Popups;

public abstract class Popup {
	protected virtual bool Resizable => true;

	protected bool cursorOverButton = false;
	protected bool isHovered = false;
	protected bool hoverRetained = false;
	protected bool collapsed = false;
	protected bool slatedForDeletion = false;
	protected UVRect closeButton;
	protected UVRect collapseButton;
	protected Rect bounds;
	protected Rect collapseBounds;
	protected Rect? initialBounds;

	protected Rect scaleControlIncluder;
	protected Rect scaleControlExcluder;
	protected float scaleControlInnerMargin = 0.02f;
	protected float scaleControlOuterMargin = 0.02f;
	protected ResizeHandlePosition resizeAnchor;
	protected bool resizingWindow = false;
	protected Vector2 resizeAnchorPoint;
	protected Vector2i resizeDirection;
	protected Vector2 currentMousePosition;
	protected Rect resizePreviewBounds;

	protected bool cursorOverrideActive = false;
	protected bool lastCursorOverrideActive = false;

	public bool Resizing => this.resizingWindow;

	public Popup() {
		this.bounds = new Rect(-0.5f, -0.5f, 0.5f, 0.5f);
		this.collapseBounds = new Rect(-0.1f, -0.1f, 0.1f, 0.1f);
		this.closeButton = new UVRect(this.bounds.x1 - 0.05f, this.bounds.y1 - 0.05f, this.bounds.x1, this.bounds.y1).UV(0f, 0f, 0.25f, 0.25f);
		this.collapseButton = new UVRect(this.bounds.x1 - 0.1f, this.bounds.y1 - 0.05f, this.bounds.x1 - 0.05f, this.bounds.y1).UV(0f, 0.5f, 0.25f, 0.75f);
		this.UpdateScaleControls(this.bounds);
	}

	private void UpdateResizing() {
		this.UpdateScaleControls(this.bounds);
		this.isHovered = this.InteractBounds().Inside(Mouse.X, Mouse.Y);
		if (this.isHovered == true) {
			this.hoverRetained = true;
		}
		bool isOutsideExcludor = !this.scaleControlExcluder.Inside(Mouse.X, Mouse.Y);
		bool isInsideIncluder = this.scaleControlIncluder.Inside(Mouse.X, Mouse.Y);
		if (this.IsDragArea(Mouse.X, Mouse.Y)) {
			return;
		}

		if (!this.collapsed && !this.cursorOverButton && !this.resizingWindow) {
			Immediate.Color(Themes.Layer2Color);
			if (this.hoverRetained && isOutsideExcludor) {
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

								this.resizeAnchor = (ResizeHandlePosition) (x + y * 3);
								switch (this.resizeAnchor) {
									case ResizeHandlePosition.Center:
										Main.mouse?.Cursor.StandardCursor = StandardCursor.ResizeAll;
										this.cursorOverrideActive = true;
										break;
									case ResizeHandlePosition.UpMid:
									case ResizeHandlePosition.DownMid:
										Main.mouse?.Cursor.StandardCursor = StandardCursor.VResize;
										this.cursorOverrideActive = true;
										break;
									case ResizeHandlePosition.MidLeft:
									case ResizeHandlePosition.MidRight:
										Main.mouse?.Cursor.StandardCursor = StandardCursor.HResize;
										this.cursorOverrideActive = true;
										break;
									case ResizeHandlePosition.UpLeft:
									case ResizeHandlePosition.DownRight:
										Main.mouse?.Cursor.StandardCursor = StandardCursor.NeswResize;
										this.cursorOverrideActive = true;
										break;
									case ResizeHandlePosition.UpRight:
									case ResizeHandlePosition.DownLeft:
										Main.mouse?.Cursor.StandardCursor = StandardCursor.NwseResize;
										this.cursorOverrideActive = true;
										break;
								}

								if (Mouse.JustLeft) {
									this.resizingWindow = true;
									this.resizeAnchorPoint = this.resizeAnchor switch {
										ResizeHandlePosition.UpLeft => new Vector2(this.bounds.x1, this.bounds.y1),
										ResizeHandlePosition.UpMid => new Vector2(this.bounds.CenterX, this.bounds.y1),
										ResizeHandlePosition.UpRight => new Vector2(this.bounds.x0, this.bounds.y1),
										ResizeHandlePosition.MidLeft => new Vector2(this.bounds.x1, this.bounds.CenterY),
										ResizeHandlePosition.Center => new Vector2(this.bounds.CenterX, this.bounds.CenterY),
										ResizeHandlePosition.MidRight => new Vector2(this.bounds.x0, this.bounds.CenterY),
										ResizeHandlePosition.DownLeft => new Vector2(this.bounds.x1, this.bounds.y0),
										ResizeHandlePosition.DownMid => new Vector2(this.bounds.CenterX, this.bounds.y0),
										ResizeHandlePosition.DownRight => new Vector2(this.bounds.x0, this.bounds.y0),
										_ => new Vector2(0, 0)
									};
									this.resizeDirection = new Vector2i(float.Sign((Mouse.Pos - this.resizeAnchorPoint).x), float.Sign((Mouse.Pos - this.resizeAnchorPoint).y));
								}
							}
						}
					}
				}
				else {
					this.hoverRetained = false;
				}
			}
			else {
				if (Settings.DEBUGVisiblePopupVisuals)
					UI.StrokeRect(this.scaleControlExcluder.x0, this.scaleControlExcluder.y0, this.scaleControlExcluder.x1, this.scaleControlExcluder.y1);
			}
			if (Mouse.JustRight) {
				if (this.initialBounds != null)
					this.ResizePopup(this.initialBounds.Value);
			}
		}
		else if (this.resizingWindow) {
			this.cursorOverrideActive = true;
			if (this.initialBounds == null) {
				this.initialBounds = this.bounds;
				this.collapseBounds = this.bounds;
			}
			if (Mouse.JustRight) {
				this.resizingWindow = false;
				this.ResizePopup(this.initialBounds.Value);
			}
			if (!Mouse.Left) {
				this.resizingWindow = false;
				this.ResizePopup(this.resizePreviewBounds);
			}
			else {
				this.currentMousePosition = Mouse.Pos;
				Rect originalBounds = this.bounds;
				bool lockXScale = false;
				bool lockYScale = false;
				if (int.IsOddInteger((int) this.resizeAnchor)) {
					if (this.resizeAnchor == (ResizeHandlePosition) 1 || this.resizeAnchor == (ResizeHandlePosition) 7) {
						lockXScale = true;
					}
					else {
						lockYScale = true;
					}
				}
				Vector2 PointB = new Vector2(
					this.currentMousePosition.x,
					this.currentMousePosition.y);

				// All of this can probably be improved in terms of legibility.
				// I feel like a lot of this is expecting cases that cannot exist.
				float newX0 = lockXScale || this.resizeDirection.x > 0 ? originalBounds.x0 : Math.Min(PointB.x, this.resizeAnchorPoint.x);
				float newX1 = lockXScale || this.resizeDirection.x < 0 ? originalBounds.x1 : Math.Max(PointB.x, this.resizeAnchorPoint.x);
				float newY0 = lockYScale || this.resizeDirection.y > 0 ? originalBounds.y0 : Math.Min(PointB.y, this.resizeAnchorPoint.y);
				float newY1 = lockYScale || this.resizeDirection.y < 0 ? originalBounds.y1 : Math.Max(PointB.y, this.resizeAnchorPoint.y);
				if (!lockXScale) {
					if (this.resizeDirection.x > 0)
						newX1 = Math.Max(this.resizeAnchorPoint.x + (this.collapseBounds.x1 - this.collapseBounds.x0), newX1);
					else
						newX0 = Math.Min(this.resizeAnchorPoint.x - (this.collapseBounds.x1 - this.collapseBounds.x0), newX0);
				}
				if (!lockYScale) {
					if (this.resizeDirection.y > 0)
						newY1 = Math.Max(this.resizeAnchorPoint.y + (this.collapseBounds.y1 - this.collapseBounds.y0), newY1);
					else
						newY0 = Math.Min(this.resizeAnchorPoint.y - (this.collapseBounds.y1 - this.collapseBounds.y0), newY0);
				}

				this.resizePreviewBounds = new Rect(newX0, newY0, newX1, newY1);
				if (Settings.DEBUGVisiblePopupVisuals) {
					Immediate.Color(Themes.BorderHighlight);
					UI.StrokeRect(originalBounds);
					UI.StrokeCircle(new Vector2(originalBounds.x0, originalBounds.y0), 0.02f, 8);
					Immediate.Color(Themes.Layer2Color);
					UI.StrokeCircle(PointB, 0.02f, 8);
					UI.StrokeCircle(this.resizeAnchorPoint + (Vector2) this.resizeDirection * 0.1f, 0.02f, 8);
					Immediate.Color(Themes.Layer1Color);
					UI.StrokeCircle(this.currentMousePosition, 0.02f, 8);
					Immediate.Color(Themes.Layer0Color);
					UI.StrokeCircle(this.resizeAnchorPoint, 0.02f, 8);
					Immediate.Color(Themes.RoomAir);
					UI.StrokeCircle(new Vector2(newX0, newY0), 0.02f, 8);
				}
				Immediate.Color(Themes.BorderHighlight);
				UI.StrokeRect(this.resizePreviewBounds);
			}
		}
	}

	public virtual void Draw() {
		if (this.closeButton.Inside(Mouse.Pos) || this.collapseButton.Inside(Mouse.Pos)) {
			this.cursorOverButton = true;
		}

		if (!this.collapsed) {
			Immediate.Color(Themes.Popup);
			UI.ButtonFillRect(this.bounds.x0, this.bounds.y0, this.bounds.x1, this.bounds.y1);
		}

		Immediate.Color(Themes.PopupHeader);
		UI.ButtonFillRect(this.bounds.x0, this.bounds.y1 - 0.05f, this.bounds.x1, this.bounds.y1);

		this.closeButton = new UVRect(this.bounds.x1 - 0.05f, this.bounds.y1 - 0.05f, this.bounds.x1, this.bounds.y1).UV(0f, 0f, 0.25f, 0.25f);
		if (UI.TextureButton(this.closeButton)) {
			this.Close();
		}

		this.collapseButton = new UVRect(this.bounds.x1 - 0.1f, this.bounds.y1 - 0.05f, this.bounds.x1 - 0.05f, this.bounds.y1);
		if (this.collapsed) {
			this.collapseButton.UV(0.25f, 0.5f, 0.5f, 0.75f);
		}
		else {
			this.collapseButton.UV(0f, 0.5f, 0.25f, 0.75f);
		}

		if (UI.TextureButton(this.collapseButton)) {
			this.collapsed = !this.collapsed;
		}

		Immediate.Color(this.isHovered ? Themes.BorderHighlight : Themes.Border);
		if (this.collapsed) {
			UI.ButtonStrokeRect(this.bounds.x0, this.bounds.y1 - 0.05f, this.bounds.x1, this.bounds.y1);
		}
		else {
			UI.ButtonStrokeRect(this.bounds.x0, this.bounds.y0, this.bounds.x1, this.bounds.y1);
		}

		if (this.IsDragArea(Mouse.X, Mouse.Y)) {
			Main.mouse?.Cursor.StandardCursor = StandardCursor.ResizeAll;
			this.cursorOverrideActive = true;
		}

		if (this.Resizable) {
			this.UpdateResizing();
		}
		else if (this.initialBounds != null) {
			this.ResizePopup(this.initialBounds.Value);
		}

		this.cursorOverButton = false;
	}

	public void FinishDraw() {
		if (this.slatedForDeletion) {
			this.cursorOverrideActive = false;
		}

		if (!this.cursorOverrideActive && this.lastCursorOverrideActive) {
			Main.mouse?.Cursor.StandardCursor = StandardCursor.Default;
			this.lastCursorOverrideActive = false;
		}
		this.lastCursorOverrideActive = this.cursorOverrideActive;
		this.cursorOverrideActive = false;
	}

	public virtual Rect InteractBounds() => this.collapsed ? new Rect(this.bounds.x0, this.bounds.y1 - 0.05f, this.bounds.x1, this.bounds.y1) : this.bounds;

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
		if (this.cursorOverrideActive || this.lastCursorOverrideActive) {
			Main.mouse?.Cursor.StandardCursor = StandardCursor.Default;
			this.lastCursorOverrideActive = false;
		}
		PopupManager.Remove(this);
		this.slatedForDeletion = true;
	}

	public virtual void Accept() => this.Close();
	public virtual void Reject() => this.Close();

	public virtual bool IsDragArea(float mouseX, float mouseY) {
		if (this.resizingWindow)
			return false;
		if (mouseX <= this.bounds.x1 - 0.1f && mouseX >= this.bounds.x0 && mouseY <= this.bounds.y1 && mouseY >= this.bounds.y1 - 0.05f) return true;

		return false;
	}

	public void Translate(Vector2 offset) {
		this.bounds += offset;
	}

	public enum ResizeHandlePosition {
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