using Silk.NET.Windowing;

namespace FloodForge;

public static class Program {
	public static IWindow window = null!;
	public static GL gl = null!;
	public static uint _anyVao;

	public static Vector2D<int> initialDisplayResolution = new Vector2D<int>(1280, 720);

	public static void Main(string[] args) {
		WindowOptions options = WindowOptions.Default with {
			Size = initialDisplayResolution,
			Title = "FloodForge",
			VideoMode = Monitor.GetMainMonitor(null).VideoMode,
		};
		window = Window.Create(options);

		window.Load += OnLoad;
		window.Render += OnRender;

		window.Run();

		window.Dispose();
		Sfx.Cleanup();
	}

	private static void OnLoad() {
		gl = GL.GetApi(window);
		Custom.Custom.Initialize(gl);

		gl.ClearColor(0f, 0f, 0f, 1f);

		window.SetWindowIcon([ Custom.Texture.LoadRawImage("assets/icon.png") ]);

		_anyVao = gl.CreateVertexArray();

		FloodForge.Main.Initialize();
	}

	public static float Delta { get; private set; }

	private static void OnRender(double delta) {
		Delta = (float) delta;
		try {
			FloodForge.Main.Render();
		} catch (Exception ex) {
			Logger.Error(ex.ToString());
			throw;
		}
	}
}