using Silk.NET.Maths;
using Silk.NET.Windowing;
using Silk.NET.OpenGL;
using System.IO;

namespace FloodForge;

public static class Program {
	public static IWindow window = null!;
	public static GL gl = null!;
	public static uint _anyVao;

	public static void Main(string[] args) {
		WindowOptions options = WindowOptions.Default with {
			Size = new Vector2D<int>(800, 600),
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