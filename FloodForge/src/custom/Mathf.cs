using System.Runtime.CompilerServices;

namespace Custom;

public static class Mathf {
	public const float PI = 3.14159265359f;
	public const float TAU = PI * 2f;
	public const float PI_2 = PI / 2f;

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static float Clamp(float value, float min, float max) {
		return Math.Clamp(value, min, max);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static float Clamp01(float value) {
		return Math.Clamp(value, 0f, 1f);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static float Lerp(float a, float b, float t) {
		return a + (b - a) * t;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static double Lerp(double a, double b, double t) {
		return a + (b - a) * t;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static float InverseLerp(float value, float a, float b) {
		return (value - a) / (b - a);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static float Sqrt(float value) {
		return MathF.Sqrt(value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static float Sin(float value) {
		return MathF.Sin(value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static float Cos(float value) {
		return MathF.Cos(value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static double Sin(double value) {
		return Math.Sin(value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static double Cos(double value) {
		return Math.Cos(value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static int FloorDivide(int a, int b) {
		return ((a < 0) ^ (b < 0)) && (a % b != 0) ? (a / b - 1) : (a / b);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static int FloorToInt(float a) {
		return (int) a;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static int CeilToInt(float a) {
		return (int) MathF.Ceiling(a);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static int RoundToInt(float a) {
		return (int) (a + 0.5f);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static int Mod(int x, int v) {
		return ((x % v) + v) % v;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static float Mod(float x, float v) {
		return ((x % v) + v) % v;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static float Mod1(float x) {
		return ((x % 1f) + 1f) % 1f;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static float Abs(float v) {
		return Math.Abs(v);
	}
}