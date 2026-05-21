using System.Runtime.CompilerServices;

namespace Custom;

[StructLayout(LayoutKind.Sequential)]
public struct Vector2 : IEquatable<Vector2> {
	public float x;
	public float y;

	public readonly float Length => MathF.Sqrt(this.x * this.x + this.y * this.y);
	public readonly float SqrLength => this.x * this.x + this.y * this.y;

	public readonly Vector2 Normalized {
		[MethodImpl(MethodImplOptions.AggressiveInlining)]
		get {
			float length = this.Length;
			return length < 0.00001f ? Vector2.Zero : this / length;
		}
	}

	public Vector2(float x, float y) {
		this.x = x;
		this.y = y;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static Vector2 operator +(Vector2 a, Vector2 b) => new Vector2(a.x + b.x, a.y + b.y);

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static Vector2 operator -(Vector2 a, Vector2 b) => new Vector2(a.x - b.x, a.y - b.y);

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static Vector2 operator *(Vector2 a, float b) => new Vector2(a.x * b, a.y * b);

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static Vector2 operator *(float b, Vector2 a) => new Vector2(a.x * b, a.y * b);

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static Vector2 operator /(Vector2 a, float b) => new Vector2(a.x / b, a.y / b);

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static Vector2 operator -(Vector2 a) => new Vector2(-a.x, -a.y);

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static Vector2 operator *(Vector2 a, Vector2 b) => new Vector2(a.x * b.x, a.y * b.y);

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static Vector2 operator /(Vector2 a, Vector2 b) => new Vector2(a.x / b.x, a.y / b.y);

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool operator ==(Vector2 a, Vector2 b) => a.Equals(b);

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool operator !=(Vector2 a, Vector2 b) => !a.Equals(b);

	public override readonly string ToString() => $"({this.x}, {this.y})";

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public void Normalize() {
		float length = this.Length;
		if (length < 0.00001f) {
			this.x = 0f;
			this.y = 0f;
			return;
		}

		this.x /= length;
		this.y /= length;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public readonly Vector2 Rounded() => new Vector2(MathF.Round(this.x), MathF.Round(this.y));

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public void Round() {
		this.x = MathF.Round(this.x);
		this.y = MathF.Round(this.y);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static float Dot(Vector2 a, Vector2 b) => a.x * b.x + a.y * b.y;

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static float Distance(Vector2 a, Vector2 b) => (a - b).Length;

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static Vector2 Lerp(Vector2 a, Vector2 b, float t) => a + (b - a) * t;

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public readonly bool Equals(Vector2 v) {
		const float epsilon = 0.000001f;
		return MathF.Abs(this.x - v.x) < epsilon && MathF.Abs(this.y - v.y) < epsilon;
	}

	public override readonly bool Equals(object? obj) => obj is Vector2 v && this.Equals(v);

	public override readonly int GetHashCode() => HashCode.Combine(this.x, this.y);

	public static implicit operator System.Numerics.Vector2(Vector2 v) => Unsafe.As<Vector2, System.Numerics.Vector2>(ref v);
	public static implicit operator Vector2(System.Numerics.Vector2 v) => Unsafe.As<System.Numerics.Vector2, Vector2>(ref v);
	public static implicit operator Vector2(Vector2D<int> v) => new(v.X, v.Y);

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static Vector2 Min(Vector2 a, Vector2 b) => new Vector2(MathF.Min(a.x, b.x), MathF.Min(a.y, b.y));

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static Vector2 Max(Vector2 a, Vector2 b) => new Vector2(MathF.Max(a.x, b.x), MathF.Max(a.y, b.y));

	public static Vector2 Zero => new Vector2(0f, 0f);
	public static Vector2 One => new Vector2(1f, 1f);
	public static Vector2 NegX => new Vector2(-1f, 1f);
	public static Vector2 NegY => new Vector2(1f, -1f);
}