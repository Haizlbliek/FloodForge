using Silk.NET.Input;

namespace FloodForge;

public static class Mouse {
	public static float LastX { get; private set; }
	public static float LastY { get; private set; }
	public static float X { get; private set; }
	public static float Y { get; private set; }
	public static bool Moved => LastX != X && LastY != Y;

	public static bool Disabled;

	public static bool Left = false;
	public static bool Right = false;
	public static bool Middle = false;
	public static bool LastLeft = false;
	public static bool LastRight = false;
	public static bool LastMiddle = false;
	public static bool JustLeft => Left && !LastLeft;
	public static bool JustRight => Right && !LastRight;
	public static bool JustMiddle => Middle && !LastMiddle;
	public static float Scroll { get; private set; } = 0f;

	private static float ScrollAccumulator = 0f;

	public static void Scrolled(ScrollWheel wheel) {
		ScrollAccumulator += wheel.Y;
	}

	public static void Update(IMouse mouse, float x, float y) {
		LastLeft = Left;
		LastRight = Right;
		LastMiddle = Middle;
		LastX = X;
		LastY = Y;

		Left = mouse.IsButtonPressed(MouseButton.Left);
		Right = mouse.IsButtonPressed(MouseButton.Right);
		Middle = mouse.IsButtonPressed(MouseButton.Middle);
		X = x;
		Y = y;

		Scroll = ScrollAccumulator;
		ScrollAccumulator = 0f;
	}

	public static Vector2 Pos => new Vector2(X, Y);
}