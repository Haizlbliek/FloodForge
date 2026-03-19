namespace FloodForge;

public abstract class MenuItems {
	protected Button[] buttons = [];

	public void Draw() {
		Rect rect = new Rect(-Main.screenBounds.x, Main.screenBounds.y, Main.screenBounds.x, Main.screenBounds.y - 0.06f);

		Immediate.Color(Themes.Popup);
		UI.FillRect(rect);

		Immediate.Color(Themes.Border);
		UI.Line(rect.x0, rect.y0, rect.x1, rect.y0);

		float x = -Main.screenBounds.x + 0.01f;
		foreach (Button button in this.buttons) {
			float width = UI.font.Measure(button.text, 0.03f).x + 0.02f;
			UI.TextButtonMods mods = new UI.TextButtonMods();
			if (button.Dark) {
				mods.textColor = Themes.TextDisabled;
			}
			if (UI.TextButton(button.text, Rect.FromSize(x, Main.screenBounds.y - 0.05f, width, 0.04f), mods)) {
				button.onclick(button);
			}
			x += width + 0.01f;
		}
	}

	protected class Button {
		public string text;
		public Action<Button> onclick;
		public virtual bool Dark => false;

		public Button(string text, Action<Button> callback) {
			this.text = text;
			this.onclick = callback;
		}
	}
}