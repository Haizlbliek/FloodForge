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
	}

	private static void OnLoad() {
		gl = GL.GetApi(window);
		Custom.Custom.Initialize(gl);

		gl.ClearColor(0f, 0f, 0f, 1f);

		window.SetWindowIcon([ Custom.Texture.LoadRawImage("assets/icon.png") ]);

		_anyVao = gl.CreateVertexArray();

		FloodForge.Main.Initialize();
	}

	private static double time = 0.0;

	private static void OnRender(double delta) {
		time += delta;
		if (time >= 1.0 / 60.0) {
			try {
				FloodForge.Main.Render();
			} catch (Exception ex) {
				Logger.Error(ex.ToString());
				throw;
			}
			time -= 1.0 / 60.0;
		}
	}
}