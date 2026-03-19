namespace FloodForge.Popups;

public class InfoPopup : Popup {
	protected string[] text;

	public InfoPopup(string text) {
		this.text = text.Split(['\n', '\r'], StringSplitOptions.RemoveEmptyEntries);
		float height = MathF.Max(0.2f, this.text.Length * 0.05f + 0.07f);
		float textWidth = this.text.Length > 0 ? this.text.Max(line => UI.font.Measure(line, 0.04f).x) : 0f;
		float width = MathF.Max(0.4f, textWidth + 0.05f);
		this.bounds = new Rect(width * -0.5f, height * -0.5f, width * 0.5f, height * 0.5f);
	}

	public override void Draw() {
		base.Draw();

		if (this.minimized) return;

		Immediate.Color(Themes.Text);

		for (int idx = 0; idx < this.text.Length; idx++) {
			float y = -((idx - this.text.Length * 0.5f) * 0.05f) - 0.04f + this.bounds.CenterY;
			UI.font.Write(this.text[idx], this.bounds.CenterX, y, 0.04f, Font.Align.TopCenter);
		}
	}
}