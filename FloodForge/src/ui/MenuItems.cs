using FloodForge.Popups;

namespace FloodForge;

public abstract class MenuItems {
	protected Button[] buttons = [];
	public Rect menuBarRect;

	public void Draw() {
		this.menuBarRect = new Rect(-Main.screenBounds.x, Main.screenBounds.y, Main.screenBounds.x, Main.screenBounds.y - 0.06f);

		Immediate.Color(Themes.Popup);
		UI.FillRect(this.menuBarRect);

		Immediate.Color(Themes.Border);
		UI.Line(this.menuBarRect.x0, this.menuBarRect.y0, this.menuBarRect.x1, this.menuBarRect.y0);

		float leftX = -Main.screenBounds.x + 0.01f;
		float rightX = Main.screenBounds.x - 0.01f;

		foreach (Button button in this.buttons) {
			if (button.hasContextCheckCallback) {
				button.buttonEnabled = button.contextCheckCallback(button);
			}
			if (button.buttonEnabled || Settings.DisabledButtonsMode.value != Settings.STDisabledButtonsMode.Hide ) {
				bool onRight = (button is AlignedButton alignedButton) && alignedButton.alignment;
				float width = UI.font.Measure(button.text, 0.03f).x + 0.02f;
				UI.TextButtonMods mods = new UI.TextButtonMods();
				if (button.Dark || (!button.buttonEnabled && Settings.DisabledButtonsMode.value == Settings.STDisabledButtonsMode.Grey)) {
					mods.textColor = Themes.TextDisabled;
				}
				if (UI.TextButton(button.text, Rect.FromSize(onRight ? rightX - width : leftX, Main.screenBounds.y - 0.05f, width, 0.04f), mods)) {
					if(button.buttonEnabled)
						button.onclick(button);
					else if (button.disabledInteractMessage != "") {
						PopupManager.Add(new InfoPopup(button.disabledInteractMessage));
					}
				}
				if(onRight) rightX -= width + 0.01f;
				else leftX += width + 0.01f;
			}
		}
	}
	
	protected class AlignedButton : Button {
		public readonly bool alignment;
		public AlignedButton(string text, bool alignment, Action<Button> callback) : base (text, callback) {
			this.alignment = alignment;
		}
		
		public AlignedButton(string text, bool alignment, Action<Button> callback, Func<Button, bool> contextCheckCallback, string disabledInteractMessage = "") : base (text, callback, contextCheckCallback, disabledInteractMessage) {
			this.alignment = alignment;
		}
	}

	protected class Button {
		public string text;
		public Action<Button> onclick;
		public bool hasContextCheckCallback = false;
		public Func<Button, bool> contextCheckCallback;
		public bool buttonEnabled = true;
		public string disabledInteractMessage;
		public virtual bool Dark => false;

		public Button(string text, Action<Button> callback) {
			this.text = text;
			this.onclick = callback;
			this.contextCheckCallback = new Func<Button, bool>(button => { return true; });
			this.hasContextCheckCallback = false;
			this.disabledInteractMessage = "";
		}

		public Button (string text, Action<Button> callback, Func<Button, bool> contextCheckCallback, string disabledInteractMessage = "") : this(text, callback) {
			this.hasContextCheckCallback = true;
			this.contextCheckCallback = contextCheckCallback;
			this.disabledInteractMessage =disabledInteractMessage;
		}
	}
}