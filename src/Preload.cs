namespace FloodForge;

public static class Preload {
	public static Shader RoomShader = Shader.Load("assets/shaders/room.vert", "assets/shaders/room.frag");
	public static Shader HueSliderShader = Shader.Load("assets/shaders/default.vert", "assets/shaders/hue_slider.frag");
	public static Shader ColorSquareShader = Shader.Load("assets/shaders/default.vert", "assets/shaders/color_square.frag");

	public static void Initialize() {
	}
}