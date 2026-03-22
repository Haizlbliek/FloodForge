namespace FloodForge.Popups;

public static class PopupManager {
	private static readonly List<Popup> trash = [];
	private static readonly List<Popup> toAdd = [];
	private static Popup? holdingPopup = null;
	private static Popup? mousePopup = null;
	private static Popup? interactingPopup = null;
	private static Vector2 holdingStart;

	public static List<Popup> Windows { get; private set; } = [];

	public static void Initialize() {
	}

	public static void Cleanup() {
		foreach (Popup popup in toAdd) {
			Windows.Add(popup);
		}
		foreach (Popup popup in trash) {
			Windows.Remove(popup);
		}

		toAdd.Clear();
		trash.Clear();
	}

	public static void Block() {
		if (!Mouse.Left && !Mouse.Right && !Mouse.Middle) {
			interactingPopup = null;
		}

		for (int i = Windows.Count - 1; i >= 0; i--) {
			Popup popup = Windows[i];

			if (popup.InteractBounds().Inside(Mouse.X, Mouse.Y)) {
				if (Mouse.JustLeft || Mouse.JustRight || Mouse.JustMiddle) {
					interactingPopup = popup;
				}
				break;
			}
			if (popup.Resizing) {
				interactingPopup = popup;
				break;
			}
		}

		Mouse.Disabled = interactingPopup != null;
	}

	public static void Draw() {
		Mouse.Disabled = false;

		for (int i = Windows.Count - 1; i >= 0; i--) {
			Popup popup = Windows[i];

			if (popup.InteractBounds().Inside(Mouse.X, Mouse.Y) || popup == interactingPopup) {
				if (Mouse.JustLeft && popup.CanDrag(Mouse.X, Mouse.Y)) {
					if (Main.AprilFools) Sfx.Play($"assets/objects/click{new Random().Next(1, 3)}.wav");
					holdingPopup = popup;
					holdingStart = Mouse.Pos;
				}

				mousePopup = popup;
				break;
			}
		}

		if (holdingPopup != null) {
			if (Mouse.Left) {
				holdingPopup.Offset(Mouse.Pos - holdingStart);
				holdingStart = Mouse.Pos;
			} else {
				holdingPopup = null;
			}
		}

		foreach (Popup popup in Windows) {
			Mouse.Disabled = popup != mousePopup;
			popup.Draw();
			popup.FinishDraw();
		}
		Mouse.Disabled = interactingPopup != null;
	}

	public static void Add(Popup popup) {
		toAdd.Add(popup);
		if (Main.AprilFools) Sfx.Play($"assets/objects/new.wav");
	}

	public static void Remove(Popup popup) {
		trash.Add(popup);
	}

	public static bool Has<T>() where T : Popup => Windows.Any(x => x is T);
}