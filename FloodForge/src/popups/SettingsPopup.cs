namespace FloodForge.Popups;

public class SettingsPopup : Popup {
	protected const float SettingSpacing = 0.02f;
	protected const float SettingHeight = 0.05f;
	protected const float SettingWidth = 0.6f;

	public SettingContainer[] settingContainers;
	protected Rect usableBounds;
	public SettingsPopup(SettingContainer[] settings) {
		this.settingContainers = settings;
		this.RecalculateBounds();
	}

	private void RecalculateBounds(bool retainTopLeft = false) {
		Vector2 TopLeft = new (this.bounds.x0, this.bounds.y1);
		float totalHeight = 0;
		float maxWidth = UI.font.Measure(this.popupTitle, 0.03f).x + 0.1f;
		foreach (SettingContainer settingContainer in this.settingContainers) {
			totalHeight += (totalHeight != 0 ? SettingSpacing : 0) + settingContainer.SettingHeight;
			maxWidth = Math.Max(maxWidth, settingContainer.SettingWidth);
		}
		totalHeight += 0.05f + SettingSpacing + 0.02f;
		maxWidth += 0.02f;
		Vector2 BottomLeft = new (TopLeft.x, TopLeft.y - totalHeight);
		this.bounds = retainTopLeft ? Rect.FromSize(BottomLeft, new (maxWidth, totalHeight)) : new Rect(-maxWidth * 0.5f, totalHeight * 0.5f, maxWidth * 0.5f, -totalHeight * 0.5f);
	}

	public override Popup Title(string title) {
		Popup returnPopup = base.Title(title);
		this.RecalculateBounds(true);
		return returnPopup;
	}

	public void AddSetting(SettingContainer container) {
		List<SettingContainer> newContainer = [.. this.settingContainers];
		newContainer.Add(container);
		this.settingContainers = [.. newContainer];
		this.RecalculateBounds(true);
	}

	public void RemoveSetting(SettingContainer container) {
		List<SettingContainer> newContainer = [.. this.settingContainers];
		newContainer.Remove(container);
		this.settingContainers = [.. newContainer];
		this.RecalculateBounds(true);
	}

	public override void Draw() {
		base.Draw();

		if (this.collapsed)
			return;

		this.usableBounds = new Rect(this.bounds.x0 + 0.01f, this.bounds.y0 + 0.01f, this.bounds.x1 - 0.01f, this.bounds.y1 - 0.05f - 0.01f);
		float yVal = this.usableBounds.y1 - SettingSpacing * 0.5f;
		foreach (SettingContainer container in this.settingContainers) {
			Rect bounds = new Rect(this.usableBounds.x0, yVal - container.SettingHeight, this.usableBounds.x1, yVal);
			container.Draw(bounds);
			yVal -= container.SettingHeight + SettingSpacing;
		}
	}

	public class SettingContainer {
		public string settingName;
		public virtual float SettingHeight {
			get {
				return SettingsPopup.SettingHeight;
			}
		}
		public virtual float SettingWidth {
			get {
				return SettingsPopup.SettingWidth;
			}
		}

		public SettingContainer(string name) {
			this.settingName = name;
		}

		public virtual void Draw(Rect bounds) {
			Immediate.Color(Themes.Text);
			UI.font.Write(this.settingName, bounds.x0, bounds.CenterY, 0.03f, Font.Align.MiddleLeft);
		}
	}

	public class HorizontalElement : SettingContainer {
		protected SettingContainer[] settings;
		protected float[] widthOverrides;
		protected float lastCalculatedWidth = 0f;
		protected float[] resultingWidths = [];
		protected bool hasDivider;
		protected bool forceEqualWidth = false;

		public HorizontalElement(SettingContainer[] settings, float[]? widthOverrides = null, bool hasDivider = true, bool forceEqualWidth = false) : base("") {
			this.settings = settings;
			this.widthOverrides = widthOverrides ?? [];
			this.forceEqualWidth = forceEqualWidth;
			this.hasDivider = hasDivider;
		}

		void RecalculateWidths(float totalWidth) {
			// - list with only overrides filled in
			this.resultingWidths = new float[this.settings.Length];
			// get remaining width
			float totalMarginlessWidth = totalWidth - SettingsPopup.SettingSpacing * (this.settings.Length - 1);
			float remainingSpace = totalMarginlessWidth;
			int unOverriddenCount = 0;
			for (int i = 0; i < this.settings.Length; i++) {
				if (i >= this.widthOverrides.Length || this.widthOverrides[i] == 0)
					unOverriddenCount++;
				else {
					this.resultingWidths[i] = this.widthOverrides[i];
					remainingSpace -= this.widthOverrides[i];
				}
			}
			// if forceEqualWidths: divide evenly
			if (this.forceEqualWidth) {
				float dividedWidth = remainingSpace / unOverriddenCount;
				for (int i = 0; i < this.resultingWidths.Length; i++) {
					if (this.resultingWidths[i] == 0)
						this.resultingWidths[i] = dividedWidth;
				}
			}
			// else: 
			else {
				//	get each individual setting's width
				List<float> settingWidths = [];
				float totalSettingWidth = 0f;
				for (int i = 0; i < this.settings.Length; i++) {
					if (this.resultingWidths[i] == 0) {
						float width = this.settings[i].SettingWidth;
						settingWidths.Add(width);
						totalSettingWidth += width;
					}
				}
				float widthFactor = remainingSpace / totalSettingWidth;
				// 	scale widths to fit remaining width
				int j = 0;
				for (int i = 0; i < this.resultingWidths.Length; i++) {
					if (i >= this.widthOverrides.Length || this.widthOverrides[i] == 0) {
						this.resultingWidths[i] = settingWidths[j] * widthFactor;
						j++;
					}
				}
			}
			this.lastCalculatedWidth = totalWidth;
		}

		public override void Draw(Rect bounds) {
			if (this.lastCalculatedWidth != bounds.x1 - bounds.x0)
				this.RecalculateWidths(bounds.x1 - bounds.x0);
			float currentXPosition = bounds.x0;
			for (int i = 0; i < this.settings.Length; i++) {
				float x1 = currentXPosition + this.resultingWidths[i];
				Rect newBounds = new Rect(currentXPosition, bounds.y0, x1, bounds.y1);
				this.settings[i].Draw(newBounds);
				if (i != 0 && this.hasDivider) {
					float lineX = currentXPosition - (SettingsPopup.SettingSpacing / 2);
					float lineHeight = SettingsPopup.SettingSpacing + SettingsPopup.SettingHeight;
					Immediate.Color(Themes.Border);
					UI.Line(lineX, bounds.CenterY - lineHeight / 2, lineX, bounds.CenterY + lineHeight / 2);
				}
				currentXPosition = x1 + SettingsPopup.SettingSpacing;
			}
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

	// Minimum seems exclusive - slider only goes to -1 with minimum set to -2
	public class IntSliderSettingContainer(string name, int initialValue, int minimum, int maximum, Action<int> callback) : SettingContainer(name) {
		protected UI.SliderIntEditable valueSlider = new UI.SliderIntEditable(minimum, maximum);
		protected int value = initialValue;
		protected bool updateWhileDragging = true;
		protected bool updateWhileUnchanged = false;
		protected Action<int> callback = callback;

		public IntSliderSettingContainer UpdateWhileDragging(bool updateWhileDragging = true) {
			this.updateWhileDragging = updateWhileDragging;
			return this;
		}

		public IntSliderSettingContainer UpdateWhileUnchanged(bool updateWhileSame = false) {
			this.updateWhileUnchanged = updateWhileSame;
			return this;
		}

		public override void Draw(Rect bounds) {
			base.Draw(bounds);
			float textWidth = UI.font.Measure(this.settingName, 0.03f).x;
			UVRect rect = new UVRect(bounds.x0 + textWidth + 0.02f, bounds.y1, bounds.x1, bounds.y0);
			int previousValue = this.value;
			UI.SliderResponse slider = UI.Slider(Rect.FromSize(rect.x0, rect.y0 + 0.02f, rect.x1 - rect.x0 - 0.04f, rect.y1 - rect.y0 - 0.04f), this.valueSlider, ref this.value, new UI.SliderMods() { disabled = false });
			Immediate.Color(Themes.Text);
			bool swap = slider.sliderPos.x > rect.x1 + 0.1f;
			float x = slider.sliderPos.x + (swap ? -0.01f : 0.01f);
			UI.font.Write($"{this.value}", x, slider.sliderPos.y, 0.03f, swap ? Font.Align.MiddleRight : Font.Align.MiddleLeft);
			if ((slider.submitted & !slider.dragging) || (slider.dragging && this.updateWhileDragging && (this.value != previousValue || this.updateWhileUnchanged))) {
				this.callback(this.value);
			}
		}
	}

	public class FloatSliderSettingContainer(string name, float initialValue, float minimum, float maximum, Action<float> callback) : SettingContainer(name) {
		protected UI.SliderFloatEditable valueSlider = new UI.SliderFloatEditable(minimum, maximum);
		protected float value = initialValue;
		protected bool updateWhileDragging = true;
		protected bool updateWhileUnchanged = false;
		protected Action<float> callback = callback;

		public FloatSliderSettingContainer UpdateWhileDragging(bool updateWhileDragging = true) {
			this.updateWhileDragging = updateWhileDragging;
			return this;
		}

		public FloatSliderSettingContainer UpdateWhileUnchanged(bool updateWhileSame = true) {
			this.updateWhileUnchanged = updateWhileSame;
			return this;
		}

		public override void Draw(Rect bounds) {
			base.Draw(bounds);
			float textWidth = UI.font.Measure(this.settingName, 0.03f).x;
			UVRect rect = new UVRect(bounds.x0 + textWidth + 0.02f, bounds.y1, bounds.x1, bounds.y0);
			float previousValue = this.value;
			UI.SliderResponse slider = UI.Slider(Rect.FromSize(rect.x0, rect.y0 + 0.02f, rect.x1 - rect.x0 - 0.04f, rect.y1 - rect.y0 - 0.04f), this.valueSlider, ref this.value, new UI.SliderMods() { disabled = false });
			Immediate.Color(Themes.Text);
			bool swap = slider.sliderPos.x > rect.x1 + 0.1f;
			float x = slider.sliderPos.x + (swap ? -0.01f : 0.01f);
			UI.font.Write($"{this.value}", x, slider.sliderPos.y, 0.03f, swap ? Font.Align.MiddleRight : Font.Align.MiddleLeft);
			if ((slider.submitted & !slider.dragging) || (slider.dragging && this.updateWhileDragging && (this.value != previousValue || this.updateWhileUnchanged))) {
				this.callback(this.value);
			}
		}
	}

	public class StringSettingContainer : SettingContainer {
		protected UI.TextInputEditable stringInput = new UI.TextInputEditable(UI.TextInputEditable.Type.Text, "");
		protected Action<string> callback;
		protected string prefix;
		protected float prefixSizeX;
		protected string hint;

		public StringSettingContainer(string name, Action<string> callback, string prefix = "", string hint = "", string postfix = "") : base(name) {
			this.callback = callback;
			this.prefix = prefix;
			this.prefixSizeX = UI.font.Measure(this.prefix, 0.03f).x;
			this.hint = hint;
		}

		public StringSettingContainer(string name, Action<string> callback, ref Action<string>? onValueChangeEvent, string prefix = "", string hint = "", string postfix = "") : this(name, callback, prefix, hint, postfix) {
			onValueChangeEvent += this.UpdateValue;
		}

		public void UpdateValue(string newValue) {
			this.stringInput.value = newValue;
		}

		public override void Draw(Rect bounds) {
			base.Draw(bounds);

			Immediate.Color(Themes.Text);
			float xPos = bounds.x0 + UI.font.Measure(this.settingName, 0.03f).x + (this.settingName != "" ? 0.02f : 0);
			UI.font.Write(this.prefix, xPos, bounds.CenterY, 0.03f, Font.Align.MiddleLeft);
			xPos += this.prefixSizeX;
			UI.TextInputResponse inputResponse = UI.TextInput(new Rect(xPos, bounds.y0, bounds.x1, bounds.y1), this.stringInput);
			if (inputResponse.submitted) {
				this.callback(this.stringInput.value);
			}
			if (!inputResponse.focused && this.stringInput.value == "") {
				Immediate.Color(Themes.TextDisabled);
				UI.font.Write(this.hint, xPos + 0.01f, bounds.CenterY, 0.03f, Font.Align.MiddleLeft);
			}
		}
	}

	public class ButtonSettingContainer : SettingContainer {
		readonly Action onClickCallback;
		Func<ButtonSettingContainer, bool>? contextCheckCallback;
		bool darkenOnFalse;

		public override float SettingWidth {
			get {
				return UI.font.Measure(this.settingName, 0.03f).x + 0.01f;
			}
		}

		public ButtonSettingContainer(string name, Action onClickCallback) : base(name) {
			this.onClickCallback = onClickCallback;
		}

		public ButtonSettingContainer SetContextCheck(Func<ButtonSettingContainer, bool> contextCheckCallback, bool darkenOnFalse = false) {
			this.contextCheckCallback = contextCheckCallback;
			this.darkenOnFalse = darkenOnFalse;
			return this;
		}

		public override void Draw(Rect bounds) {
			bool enabled = (this.contextCheckCallback == null) || this.contextCheckCallback(this);
			if (UI.TextButton(this.settingName, bounds, new UI.TextButtonMods((enabled || !this.darkenOnFalse) ? Themes.Text : Themes.TextDisabled)) && enabled) {
				this.onClickCallback();
			}
		}
	}

	public class LabelContainer : SettingContainer {
		public override float SettingWidth {
			get {
				return UI.font.Measure(this.settingName, 0.03f).x;
			}
		}

		public LabelContainer(string name) : base(name) {}

		public override void Draw(Rect bounds) {
			Immediate.Color(Themes.Text);
			UI.font.Write(this.settingName, bounds.CenterX, bounds.CenterY, 0.03f, Font.Align.MiddleCenter);
		}
	}

	public class Divider : SettingContainer {
		public override float SettingHeight => 0.01f;
		public override float SettingWidth => 0.1f;

		public Divider() : base("") {}

		public override void Draw(Rect bounds) {
			Immediate.Color(Themes.BorderHighlight);
			UI.Line(bounds.x0, bounds.CenterY, bounds.x1, bounds.CenterY);
		}
	}
}