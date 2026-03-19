namespace Custom;

[StructLayout(LayoutKind.Sequential)]
public struct Vector2 {
	public float x;
	public float y;

	public readonly float Length => MathF.Sqrt(this.x * this.x + this.y * this.y);
	public readonly float SqrLength => this.x * this.x + this.y * this.y;
	public readonly Vector2 Normalized {
		get {
			float length = this.Length;
			return length < 0.00001f ? Vector2.Zero : this / length;
		}
	}

	public Vector2(float x, float y) {
		this.x = x;
		this.y = y;
	}

	public static Vector2 operator +(Vector2 a, Vector2 b) {
		return new Vector2(a.x + b.x, a.y + b.y);
	}

	public static Vector2 operator -(Vector2 a, Vector2 b) {
		return new Vector2(a.x - b.x, a.y - b.y);
	}

	public static Vector2 operator *(Vector2 a, float b) {
		return new Vector2(a.x * b, a.y * b);
	}

	public static Vector2 operator *(float b, Vector2 a) {
		return new Vector2(a.x * b, a.y * b);
	}

	public static Vector2 operator /(Vector2 a, float b) {
		return new Vector2(a.x / b, a.y / b);
	}

	public static Vector2 operator -(Vector2 a) {
		return new Vector2(-a.x, -a.y);
	}

	public static Vector2 operator *(Vector2 a, Vector2 b) {
		return new Vector2(a.x * b.x, a.y * b.y);
	}

	public static Vector2 operator /(Vector2 a, Vector2 b) {
		return new Vector2(a.x / b.x, a.y / b.y);
	}

	public override readonly string ToString() {
		return $"({this.x}, {this.y})";
	}

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

	public readonly Vector2 Rounded() {
		return new Vector2(MathF.Round(this.x), MathF.Round(this.y));
	}

	public void Round() {
		this.x = MathF.Round(this.x);
		this.y = MathF.Round(this.y);
	}

	public static float Dot(Vector2 a, Vector2 b) {
		return a.x * b.x + a.y * b.y;
	}

	public static float Distance(Vector2 a, Vector2 b) {
		return (a - b).Length;
	}

	public static Vector2 Lerp(Vector2 a, Vector2 b, float t) {
		return a + (b - a) * t;
	}

	public override readonly bool Equals(object? obj) {
		return obj is Vector2 v && this.x == v.x && this.y == v.y;
	}

	public override readonly int GetHashCode() {
		return HashCode.Combine(this.x, this.y);
	}

	public static implicit operator System.Numerics.Vector2(Vector2 v) {
		return new System.Numerics.Vector2(v.x, v.y);
	}

	public static implicit operator Vector2(System.Numerics.Vector2 v) {
		return new Vector2(v.X, v.Y);
	}

	public static implicit operator Vector2(Vector2D<int> v) {
		return new Vector2(v.X, v.Y);
	}

	public static Vector2 Min(Vector2 a, Vector2 b) {
		return new Vector2(MathF.Min(a.x, b.x), MathF.Min(a.y, b.y));
	}

	public static Vector2 Max(Vector2 a, Vector2 b) {
		return new Vector2(MathF.Max(a.x, b.x), MathF.Max(a.y, b.y));
	}

	public static Vector2 Zero => new Vector2(0f, 0f);
	public static Vector2 One => new Vector2(1f, 1f);
	public static Vector2 NegY => new Vector2(1f, -1f);
}