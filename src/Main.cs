using FloodForge.Droplet;
using FloodForge.Popups;
using FloodForge.World;
using Silk.NET.Input;

namespace FloodForge;

public static class Main {
	public static Random random = new Random();
	public static IInputContext input = null!;

	public static Vector2 GlobalMouse { get; private set; }
	public static bool fullscreen = false;
	public static Vector2 screenBounds;

	public static Mode mode = Mode.World;

	public static bool Anniversary = false;
	public static bool AprilFools = false;

	public static event Action<float, float> Scroll = (x, y) => {};
	public static event Action<Key> KeyPress = (key) => {};

	public static void Initialize() {
		DateTime now = DateTime.UtcNow;
		Anniversary = now.Year == 2025 && now.Month == 11 && now.Day < 22;
		AprilFools = now.Month == 4 && now.Day == 1;

		Logger.Info("Initializing...");
		Preload.Initialize();
		Settings.Initialize();
		Immediate.Initialize();
		WorldWindow.Initialize();
		DropletWindow.Initialize();
		PopupManager.Initialize();

		input = Program.window.CreateInput();
		for (int i = 0; i < input.Keyboards.Count; i++) {
			var keyboard = input.Keyboards[i];
			keyboard.KeyDown += KeyDown;
			keyboard.KeyUp += KeyUp;
		}
	
		IMouse? mouse = input.Mice.FirstOrDefault();
		mouse?.Scroll += OnScroll;

		UI.Initialize();

		PopupManager.Add(new SplashArtPopup());
	}

	private static void OnScroll(IMouse mouse, ScrollWheel wheel) {
		Mouse.Scrolled(wheel);
		Scroll(wheel.X, wheel.Y);
	}

	private static void KeyDown(IKeyboard keyboard, Key key, int arg3) {
		Keys.Press(key);
		KeyPress(key);
		UI.KeyPress(key);
	}

	private static void KeyUp(IKeyboard keyboard, Key key, int arg3) {
		Keys.Release(key);
	}

	public static void Render() {
		if (!Program.window.IsVisible) return;

		Immediate.MatrixMode(Immediate.EMatrixMode.PROJECTION);
		Immediate.LoadIdentity();
		float size = MathF.Min(Program.window.FramebufferSize.X, Program.window.FramebufferSize.Y);
		float offsetX = (Program.window.FramebufferSize.X * 0.5f) - size * 0.5f;
		float offsetY = (Program.window.FramebufferSize.Y * 0.5f) - size * 0.5f;
		screenBounds = ((Vector2) Program.window.FramebufferSize) / size;

		Program.gl.Viewport(0, 0, (uint) Program.window.FramebufferSize.X, (uint) Program.window.FramebufferSize.Y);
		Immediate.Ortho(-1f * screenBounds.x, 1f * screenBounds.x, -1f * screenBounds.y, 1f * screenBounds.y, 0f, 1f);

		IMouse? mouse = input.Mice.FirstOrDefault();
		if (mouse != null) {
			GlobalMouse = (Vector2) mouse.Position;
			GlobalMouse = new Vector2(
				(GlobalMouse.x - offsetX) / size * 2f - 1f,
				(GlobalMouse.y - offsetY) / size * -2f + 1f
			);
			Mouse.Update(mouse, GlobalMouse.x, GlobalMouse.y);
		}

		PopupManager.Block();

		Program.gl.StencilMask(0xFF);
		Program.gl.Clear(ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit | ClearBufferMask.StencilBufferBit);
		Program.gl.Disable(EnableCap.DepthTest);
		Program.gl.BlendFunc(BlendingFactor.SrcAlpha, BlendingFactor.OneMinusSrcAlpha);

		Immediate.Color(Themes.Background);
		UI.FillRect(-screenBounds.x, -screenBounds.y, screenBounds.x, screenBounds.y);

		if (mode == Mode.World) {
			World.WorldWindow.Draw();
		} else if (mode == Mode.Droplet) {
			Droplet.DropletWindow.Draw();
		}

		PopupManager.Draw();
		Immediate.Flush();

		PopupManager.Cleanup();

		if (Keys.JustPressed(Key.F11)) {
			fullscreen = !fullscreen;
			Program.window.WindowState = fullscreen ? Silk.NET.Windowing.WindowState.Fullscreen : Silk.NET.Windowing.WindowState.Normal;
		}

		if (Keys.JustPressed(Key.Escape)) {
			if (PopupManager.Windows.Count > 0) {
				PopupManager.Windows.Last().Reject();
			} else {
				if (mode == Mode.Droplet) {
					PopupManager.Add(new ConfirmPopup("Exit Droplet?\nUnsaved changes will be lost").Okay(() => {
						mode = Mode.World;
						DropletWindow.Reset();
					}));
				}
				else {
					PopupManager.Add(new ConfirmPopup("Exit FloodForge?").Okay(() => {
						Program.window.Close();
					}));
				}
			}
		}

		if (Keys.JustPressed(Key.Enter)) {
			if (PopupManager.Windows.Count > 0) {
				PopupManager.Windows.Last().Accept();
			}
		}

		Keys.End();
	}

	public enum Mode {
		World,
		Droplet,
	}
}