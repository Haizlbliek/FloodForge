namespace FloodForge.Popups;

public class ColorEditPopup : Popup {
	protected Color color;
	protected Action<Color> callback;
	protected float hue;
	protected bool centerFocused = false;
	protected bool sliderFocused = false;

	protected int _hueLoc, _projLocC, _modelLocC;
	protected int _projLocH, _modelLocH;

	protected Rect selectorRect;
	protected Rect sliderRect;
	protected float hueSliderSize = 0.02f;

	public ColorEditPopup(Color initialColor, Action<Color> submitCallback) {
		this.color = initialColor;
		this.callback = submitCallback;
		this.bounds = new Rect(-0.2f, -0.2f, 0.2f, 0.2f);
		Color.ToHSV(this.color, out this.hue, out _, out _);
	}

	public override void Draw() {
		if (this.selectorRect.Inside(Mouse.Pos) || this.sliderRect.Inside(Mouse.Pos))
			this.cursorOverButton = true;

		base.Draw();
		if (this.minimized) return;
		
		if (this.callback.Target == null)
			this.Close();

		Vector3 hsv = this.color.ToHsv();

		bool colorChanged = false;

		float newx0 = this.bounds.x0 + 0.01f;
		this.selectorRect = new Rect(newx0, this.bounds.y0 + 0.01f, this.bounds.x1 - 0.01f - (this.hueSliderSize + 0.03f), this.bounds.y1 - 0.06f);
		float selectorHeight = this.selectorRect.y1 - this.selectorRect.y0;
		float selectorWidth = this.selectorRect.x1 - this.selectorRect.x0;
		this.sliderRect = Rect.FromSize(this.selectorRect.x1 + 0.02f, this.selectorRect.y0, this.hueSliderSize, selectorHeight);

		Immediate.Color(Themes.Background);
		UI.FillRect(this.selectorRect);
	
		Immediate.UseProgram(Preload.ColorSquareShader.shader);
		if (this._hueLoc == 0) {
			this._hueLoc = Program.gl.GetUniformLocation(Preload.ColorSquareShader, "hue");
			this._projLocC = Program.gl.GetUniformLocation(Preload.ColorSquareShader, "projection");
			this._modelLocC = Program.gl.GetUniformLocation(Preload.ColorSquareShader, "model");
		}
		Program.gl.Uniform1(this._hueLoc, this.hue / 360f);
		Program.gl.UniformMatrix4(this._projLocC, false, [..Matrix4X4.CreateOrthographicOffCenter(-Main.screenBounds.x, Main.screenBounds.x, -Main.screenBounds.y, Main.screenBounds.y, 0f, 1f)]);
		Program.gl.UniformMatrix4(this._modelLocC, false, [..Matrix4X4.CreateTranslation(0f, 0f, 0f)]);

		Immediate.Color(1f, 1f, 1f);
		Immediate.Begin(Immediate.PrimitiveType.QUADS);
		Immediate.TexCoord(0f, 1f); Immediate.Vertex(this.selectorRect.x0, this.selectorRect.y1);
		Immediate.TexCoord(1f, 1f); Immediate.Vertex(this.selectorRect.x1, this.selectorRect.y1);
		Immediate.TexCoord(1f, 0f); Immediate.Vertex(this.selectorRect.x1, this.selectorRect.y0);
		Immediate.TexCoord(0f, 0f); Immediate.Vertex(this.selectorRect.x0, this.selectorRect.y0);
		Immediate.End();
		Immediate.UseProgram(0);
	
		if (Mouse.JustLeft && !Mouse.Disabled && this.selectorRect.Inside(Mouse.Pos)) {
			this.centerFocused = true;
		}

		if (this.centerFocused) {
			if (!Mouse.Left)
				this.centerFocused = false;

			float s = Mathf.Clamp((Mouse.X - this.bounds.x0 - 0.01f) / selectorWidth, 0f, 1f);
			float v = Mathf.Clamp((Mouse.Y - (this.bounds.y1 - 0.06f - selectorHeight)) / selectorHeight, 0f, 1f);
			hsv.x = this.hue;
			hsv.y = s;
			hsv.z = v;
			this.color = Color.FromHSV(hsv.x, hsv.y, hsv.z);
			colorChanged = true;
		}

		Vector2 colorPos = new Vector2(hsv.y * selectorWidth + 0.01f + this.bounds.x0, hsv.z * selectorHeight + this.bounds.y1 - 0.06f - selectorHeight);
		Rect colorRect = Rect.FromSize(colorPos.x - 0.01f, colorPos.y - 0.01f, 0.02f, 0.02f);
		Immediate.Color(this.color);
		UI.FillRect(colorRect);
		float val = hsv.z > 0.5f ? 0f : 1f;
		Immediate.Color(val, val, val);
		UI.StrokeRect(colorRect);

		Immediate.UseProgram(Preload.HueSliderShader.shader);
		if (this._projLocH == 0) {
			this._projLocH = Program.gl.GetUniformLocation(Preload.HueSliderShader, "projection");
			this._modelLocH = Program.gl.GetUniformLocation(Preload.HueSliderShader, "model");
		}
		Program.gl.UniformMatrix4(this._projLocH, false, [..Matrix4X4.CreateOrthographicOffCenter(-Main.screenBounds.x, Main.screenBounds.x, -Main.screenBounds.y, Main.screenBounds.y, 0f, 1f)]);
		Program.gl.UniformMatrix4(this._modelLocH, false, [..Matrix4X4.CreateTranslation(0f, 0f, 0f)]);
		Immediate.Color(1f, 1f, 1f);
		Immediate.Begin(Immediate.PrimitiveType.QUADS);
		Immediate.TexCoord(0f, 1f); Immediate.Vertex(this.sliderRect.x0, this.sliderRect.y1);
		Immediate.TexCoord(1f, 1f); Immediate.Vertex(this.sliderRect.x1, this.sliderRect.y1);
		Immediate.TexCoord(1f, 0f); Immediate.Vertex(this.sliderRect.x1, this.sliderRect.y0);
		Immediate.TexCoord(0f, 0f); Immediate.Vertex(this.sliderRect.x0, this.sliderRect.y0);
		Immediate.End();
		Immediate.UseProgram(0);

		float hueY = Mathf.Lerp(this.sliderRect.y0, this.sliderRect.y1, this.hue / 360f);
		UI.Line(this.sliderRect.x0 - 0.01f, hueY, this.sliderRect.x1 + 0.01f, hueY);

		bool sliderHover = this.sliderRect.Inside(Mouse.Pos);
		if (sliderHover) {
			Main.mouse?.Cursor.StandardCursor = Silk.NET.Input.StandardCursor.VResize;
			this.mouseCursorSet = true;
		}

		if (Mouse.JustLeft && !Mouse.Disabled && sliderHover) {
			this.sliderFocused = true;
		}
		if (this.sliderFocused) {
			if (!Mouse.Left)
				this.sliderFocused = false;

			float h = Mathf.Clamp((Mouse.Y - (this.bounds.y1 - 0.06f - selectorHeight)) / selectorHeight, 0f, 1f) * 360f;
			hsv.x = h;
			this.hue = h;
			this.color = Color.FromHSV(hsv.x, hsv.y, hsv.z);
			colorChanged = true;
		}

		if (colorChanged) {
			this.callback(this.color);
		}
	}

	public override void Close() {
		base.Close();
		if(this.callback.Target != null)
			this.callback(this.color);
	}
}