using Silk.NET.Windowing;

namespace FloodForge;

public static class Program {
	public static IWindow window = null!;
	public static GL gl = null!;
	public static uint _anyVao;

	public static Vector2D<int> initialDisplayResolution = new Vector2D<int>(1280, 720);

	public static void Main(string[] args) {
		if (File.Exists("crashlog.txt")) File.Delete("crashlog.txt");

		foreach (string arg in args) {
			if (!arg.StartsWith("--patcher=")) continue;

			string patcherFolderPath = arg[10..].TrimStart('"').TrimEnd('"');
			string patcherName = OperatingSystem.IsWindows() ? "FloodForge.Patcher.exe" : "FloodForge.Patcher";
			string patcherPath = PathUtil.Combine(patcherFolderPath, patcherName);
			if (File.Exists(patcherPath)) {
				File.Copy(patcherPath, patcherName, true);
				File.Copy(patcherPath + ".pdb", patcherName + ".pdb", true);
			}
		}

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
			Logger.Error($"{ex.Message}\n{ex.StackTrace}");
		}
	}

	private static void OnLoad() {
		gl = GL.GetApi(window);
		Custom.Custom.Initialize(gl);

		gl.ClearColor(0f, 0f, 0f, 1f);

		window.SetWindowIcon([ Custom.Texture.LoadRawImage("assets/icon.png") ]);

		_anyVao = gl.GenVertexArray();

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