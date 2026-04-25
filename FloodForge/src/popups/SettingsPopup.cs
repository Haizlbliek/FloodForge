namespace FloodForge.Popups;

public class SettingsPopup : Popup {
	public SettingContainer[] settingContainers;
	protected float settingHeight = 0.04f;
	protected float settingSpacing = 0.06f;
	protected Rect usableBounds;
	public SettingsPopup(SettingContainer[] settings) {
		this.settingContainers = settings;
		float totalHeight = 0;
		foreach (SettingContainer _ in this.settingContainers) {
			totalHeight += (totalHeight != 0 ? this.settingSpacing : 0) + this.settingHeight;
		}
		totalHeight += 0.05f + this.settingSpacing;
		this.bounds = new Rect(-0.3f, -(totalHeight * 0.5f) - 0.01f, 0.3f, (totalHeight * 0.5f) + 0.01f);
	}

	public override void Draw() {
		base.Draw();
		this.usableBounds = new Rect(this.bounds.x0 + 0.01f, this.bounds.y0 + 0.01f, this.bounds.x1 - 0.01f, this.bounds.y1 - 0.05f);
		float yVal = this.usableBounds.y1 - (this.settingSpacing + this.settingHeight) * 0.5f;
		foreach (SettingContainer container in this.settingContainers) {
			Rect bounds = new(this.usableBounds.x0, yVal - this.settingHeight, this.usableBounds.x1, yVal);
			container.Draw(bounds);
			yVal -= this.settingHeight + this.settingSpacing;
		}
	}

	public class SettingContainer {
		public string settingName;

		public SettingContainer(string name) {
			this.settingName = name;
		}

		public virtual void Draw(Rect bounds) {
			Immediate.Color(Themes.Text);
			UI.font.Write(this.settingName, bounds.x0, bounds.CenterY, 0.03f, Font.Align.MiddleLeft);
		}
	}

	public class BoolSettingContainer : SettingContainer {
		protected bool value;
		protected Action<bool> callback;

		public BoolSettingContainer(string name, bool initialValue, Action<bool> callback) : base(name) {
			this.value = initialValue;
			this.callback = callback;
		}

		public override void Draw(Rect bounds) {
			base.Draw(bounds);
			float textWidth = UI.font.Measure(this.settingName, 0.03f).x;
			if (UI.CheckBox(Rect.FromSize(bounds.x0 + textWidth + 0.02f, bounds.CenterY - 0.025f, 0.05f, 0.05f), ref this.value).clicked) {
				this.callback(this.value);
			}
		}
	}

	public class FloatSettingContainer : SettingContainer {
		protected UI.SliderFloatEditable valueSlider = new UI.SliderFloatEditable(0.05f, 1);
		protected float value;
		protected bool updateWhileDragging = true;
		protected Action<float> callback;

		public FloatSettingContainer(string name, float initialValue, float minimum, float maximum, Action<float> callback) : base(name) {
			this.value = initialValue;
			this.valueSlider.min = minimum;
			this.valueSlider.max = maximum;
			this.callback = callback;
		}

		public FloatSettingContainer UpdateWhileDragging(bool updateWhileDragging) {
			this.updateWhileDragging = updateWhileDragging;
			return this;
		}

		public override void Draw(Rect bounds) {
			base.Draw(bounds);
			float textWidth = UI.font.Measure(this.settingName, 0.03f).x;
			UVRect rect = new UVRect(bounds.x0 + textWidth + 0.02f, bounds.y1, bounds.x1, bounds.y0);
			UI.SliderResponse slider = UI.Slider(Rect.FromSize(rect.x0, rect.y0 + 0.02f, rect.x1 - rect.x0 - 0.04f, rect.y1 - rect.y0 - 0.04f), this.valueSlider, ref this.value, new UI.SliderMods() { disabled = false });
			Immediate.Color(Themes.Text);
			bool swap = slider.sliderPos.x > rect.x1 + 0.1f;
			float x = slider.sliderPos.x + (swap ? -0.01f : 0.01f);
			UI.font.Write($"{this.value}", x, slider.sliderPos.y, 0.03f, swap ? Font.Align.MiddleRight : Font.Align.MiddleLeft);
			if (slider.submitted || (slider.dragging && this.updateWhileDragging)) {
				this.callback(this.value);
			}
		}
	}
}