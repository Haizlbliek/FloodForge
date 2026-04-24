namespace FloodForge.Popups;

public class SettingsPopup : Popup {
    protected UI.SliderFloatEditable ScaleSlider = new UI.SliderFloatEditable(0.05f, 1);
    protected float Scale;
    protected float minScale, maxScale;
    protected bool updateWhileDragging = true;
    protected Action<float> callback;

    public SettingsPopup (float initValue, float minValue, float maxValue, Action<float> callback) {
        this.Scale = initValue;
        this.minScale = minValue;
        this.maxScale = maxValue;
        this.callback = callback;
        this.bounds = new Rect(-0.5f, -0.1f, 0.5f, 0.1f);
    }

    public SettingsPopup UpdateWhileDragging (bool update) {
        this.updateWhileDragging = update;
        return this;
    }

	public override void Draw() {
		base.Draw();

        string text = "Scale";
        float textWidth = UI.font.Measure(text, 0.03f).x;
        UVRect rect = new UVRect(this.bounds.x0 + textWidth + 0.03f, this.bounds.CenterY + 0.02f, this.bounds.x1 - 0.02f, this.bounds.CenterY - 0.02f);
        UI.SliderResponse slider = UI.Slider(Rect.FromSize(rect.x0, rect.y0 + 0.02f, rect.x1 - rect.x0 - 0.04f, rect.y1 - rect.y0 - 0.04f), this.ScaleSlider, ref this.Scale, new UI.SliderMods() { disabled = false });
		Immediate.Color(Themes.Text);
		bool swap = slider.sliderPos.x > rect.x1 + 0.1f;
		float x = slider.sliderPos.x + (swap ? -0.01f : 0.01f);
        UI.font.Write(text, this.bounds.x0 + 0.02f, this.bounds.CenterY, 0.03f, Font.Align.MiddleLeft);
		UI.font.Write($"{this.Scale}", x, slider.sliderPos.y, 0.03f, swap ? Font.Align.MiddleRight : Font.Align.MiddleLeft);
        if(!slider.dragging || this.updateWhileDragging) {
            this.callback(this.Scale);
        }
	}
}