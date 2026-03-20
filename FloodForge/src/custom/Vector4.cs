namespace Custom;

[StructLayout(LayoutKind.Sequential)]
public struct Vector4 {
	public float x;
	public float y;
	public float z;
	public float w;

	public readonly float Length => Mathf.Sqrt(this.x * this.x + this.y * this.y + this.z * this.z + this.w * this.w);
	public readonly float SqrLength => this.x * this.x + this.y * this.y + this.z * this.z + this.w * this.w;

	public readonly Vector4 Normalized => this / this.Length;

	public Vector4(float x, float y, float z, float w) {
		this.x = x;
		this.y = y;
		this.z = z;
		this.w = w;
	}

	public static Vector4 operator +(Vector4 a, Vector4 b) {
		return new Vector4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
	}

	public static Vector4 operator -(Vector4 a, Vector4 b) {
		return new Vector4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
	}

	public static Vector4 operator *(Vector4 a, Vector4 b) {
		return new Vector4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
	}

	public static Vector4 operator /(Vector4 a, Vector4 b) {
		return new Vector4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
	}

	public static Vector4 operator *(Vector4 a, float b) {
		return new Vector4(a.x * b, a.y * b, a.z * b, a.w * b);
	}

	public static Vector4 operator /(Vector4 a, float b) {
		return new Vector4(a.x / b, a.y / b, a.z / b, a.w / b);
	}

	public static Vector4 operator -(Vector4 a) {
		return new Vector4(-a.x, -a.y, -a.z, -a.w);
	}

	public static Vector4 Zero => new Vector4(0f, 0f, 0f, 0f);
	public static Vector4 One => new Vector4(1f, 1f, 1f, 1f);
	public static Vector4 Up => new Vector4(0f, 1f, 0f, 0f);
	public static Vector4 Down => new Vector4(0f, -1f, 0f, 0f);
	public static Vector4 Left => new Vector4(-1f, 0f, 0f, 0f);
	public static Vector4 Right => new Vector4(1f, 0f, 0f, 0f);
	public static Vector4 Forward => new Vector4(0f, 0f, 1f, 0f);
	public static Vector4 Backward => new Vector4(0f, 0f, -1f, 0f);
	public static Vector4 Ana => new Vector4(0f, 0f, 0f, 1f);
	public static Vector4 Kata => new Vector4(0f, 0f, 0f, -1f);
}