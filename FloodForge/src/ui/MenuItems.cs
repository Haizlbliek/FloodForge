using FloodForge.Popups;

namespace FloodForge;

public abstract class MenuItems {
	protected UIItem[] items = [];
	protected Rect menuBarRect;
	protected Dropdown? selectedDropdown;

	public bool Hovered() {
		return this.menuBarRect.Inside(Mouse.Pos) || this.selectedDropdown != null;
	}

	private bool DrawButton(Button button, float x, float y) {
		if (button.contextCheckCallback != null) {
			button.buttonEnabled = button.contextCheckCallback(button);
		}

		if (button.buttonEnabled || Settings.DisabledButtonsMode.value != Settings.STDisabledButtonsMode.Hide ) {
			UI.TextButtonMods mods = new UI.TextButtonMods();
			if (button.Dark || (!button.buttonEnabled && Settings.DisabledButtonsMode.value == Settings.STDisabledButtonsMode.Grey)) {
				mods.textColor = Themes.TextDisabled;
			}
			if (UI.TextButton(button.Text, Rect.FromSize(x, y, button.Size.x + 0.02f, 0.04f), mods)) {
				if (button.buttonEnabled) {
					button.onclick(button);
				}
				else if (button.disabledInteractMessage != null) {
					PopupManager.Add(new InfoPopup(button.disabledInteractMessage));
				}

				if (!button.preventClose) this.selectedDropdown = null;
			}
			return true;
		}

		return false;
	}

	public void Draw() {
		this.menuBarRect = new Rect(-Main.screenBounds.x, Main.screenBounds.y, Main.screenBounds.x, Main.screenBounds.y - 0.06f);

		Immediate.Color(Themes.Popup);
		UI.FillRect(this.menuBarRect);

		Immediate.Color(Themes.Border);
		UI.Line(this.menuBarRect.x0, this.menuBarRect.y0, this.menuBarRect.x1, this.menuBarRect.y0);

		float x = -Main.screenBounds.x + 0.01f;

		foreach (UIItem item in this.items) {
			float width = item.Size.x + 0.02f;

			if (item is Dropdown dropdown && this.selectedDropdown == dropdown) {
				float y = Main.screenBounds.y - 0.1f;

				float maxWidth = width;
				foreach (Button button in dropdown.buttons) {
					maxWidth = MathF.Max(maxWidth, button.Size.x + 0.02f);
				}
				Rect extendedRect = Rect.FromSize(x - 0.01f, y + 0.05f, maxWidth + 0.02f, -0.05f * dropdown.buttons.Count - 0.01f);

				Immediate.Color(Themes.Popup);
				UI.FillRect(extendedRect);

				Immediate.Color(Themes.Border);
				UI.Line(extendedRect.x0, extendedRect.y0, extendedRect.x0, extendedRect.y1);
				UI.Line(extendedRect.x0, extendedRect.y0, extendedRect.x1, extendedRect.y0);
				UI.Line(extendedRect.x1, extendedRect.y0, extendedRect.x1, extendedRect.y1);

				foreach (Button button in dropdown.buttons) {
					if (this.DrawButton(button, x, y)) {
						y -= 0.05f;
					}
				}
			}

			if (item is Button button1) {
				if (this.DrawButton(button1, x, Main.screenBounds.y - 0.05f)) {
					x += width + 0.01f;
				}
			}
			else if (item is Dropdown dropdown1) {
				if (UI.TextButton(item.Text, Rect.FromSize(x, Main.screenBounds.y - 0.05f, width, 0.04f), new UI.TextButtonMods { selected = this.selectedDropdown == item })) {
					this.selectedDropdown = this.selectedDropdown == dropdown1 ? null : dropdown1;
				}

				x += width + 0.01f;
			}
		}
	}

	protected abstract class UIItem {
		protected string text;
		public Vector2 Size { get; protected set; }

		public string Text {
			get => this.text;
			set {
				this.text = value;
				this.CalculateSize();
			}
		}

		protected void CalculateSize() {
			this.Size = UI.font.Measure(this.text, 0.03f);
		}

		public UIItem(string text) {
			this.text = text;
			this.CalculateSize();
		}
	}

	protected class Dropdown : UIItem {
		public readonly List<Button> buttons;

		public Dropdown(string name, List<Button> buttons) : base(name) {
			this.buttons = buttons;
		}
	}

	protected class Button : UIItem {
		public Action<Button> onclick;
		public Func<Button, bool>? contextCheckCallback;
		public bool buttonEnabled = true;
		public string? disabledInteractMessage;
		public virtual bool Dark => false;
		public bool preventClose = false;

		public Button(string text, Action<Button> callback) : base(text) {
			this.onclick = callback;
			this.contextCheckCallback = null;
		}

		public Button(string text, Action<Button> callback, Func<Button, bool> contextCheckCallback, string? disabledInteractMessage = null) : this(text, callback) {
			this.contextCheckCallback = contextCheckCallback;
			this.disabledInteractMessage = disabledInteractMessage;
		}
	}
}