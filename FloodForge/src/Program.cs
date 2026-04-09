using Silk.NET.Windowing;

namespace FloodForge;

public static class Program {
	public static IWindow window = null!;
	public static GL gl = null!;
	public static uint _anyVao;

	public static Vector2D<int> initialDisplayResolution = new Vector2D<int>(1280, 720);

	public static void Main() {
		try {
			bool isArm64 = RuntimeInformation.ProcessArchitecture == Architecture.Arm64;

			WindowOptions options = WindowOptions.Default with {
				Size = initialDisplayResolution,
				Title = "FloodForge",
				VideoMode = VideoMode.Default,
				API = isArm64
					? new GraphicsAPI(ContextAPI.OpenGL, ContextProfile.Core, ContextFlags.Default, new APIVersion(3, 3))
					: GraphicsAPI.Default,
			};
			window = Window.Create(options);

			window.Load += OnLoad;
			window.Render += OnRender;

			window.Run();

			window.Dispose();
			FloodForge.Main.Cleanup();
		} catch (Exception ex) {
			if (File.Exists("crashlog.txt")) File.Delete("crashlog.txt");

			string error = $"{ex.Message}\n{ex.StackTrace}";
			File.WriteAllText("crashlog.txt", error, System.Text.Encoding.UTF8);
		}
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
		Delta = MathF.Min((float) delta, 0.1f);
		try {
			FloodForge.Main.Render();
		} catch (Exception ex) {
			Logger.Error(ex.ToString());
			throw;
		}
	}
}