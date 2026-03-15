using System.Runtime.InteropServices;

namespace Custom;

[StructLayout(LayoutKind.Sequential)]
public struct Vector3 {
	public float x;
	public float y;
	public float z;

	public readonly float Length => Mathf.Sqrt(this.x * this.x + this.y * this.y + this.z * this.z);
	public readonly float SqrLength => this.x * this.x + this.y * this.y + this.z * this.z;

	public readonly Vector3 Normalized => this / this.Length;

	public Vector3(float x, float y, float z) {
		this.x = x;
		this.y = y;
		this.z = z;
	}

	public static Vector3 operator +(Vector3 a, Vector3 b) {
		return new Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
	}

	public static Vector3 operator -(Vector3 a, Vector3 b) {
		return new Vector3(a.x - b.x, a.y - b.y, a.z - b.z);
	}

	public static Vector3 operator *(Vector3 a, Vector3 b) {
		return new Vector3(a.x * b.x, a.y * b.y, a.z * b.z);
	}

	public static Vector3 operator /(Vector3 a, Vector3 b) {
		return new Vector3(a.x / b.x, a.y / b.y, a.z / b.z);
	}

	public static Vector3 operator *(Vector3 a, float b) {
		return new Vector3(a.x * b, a.y * b, a.z * b);
	}

	public static Vector3 operator /(Vector3 a, float b) {
		return new Vector3(a.x / b, a.y / b, a.z / b);
	}

	public static Vector3 operator -(Vector3 a) {
		return new Vector3(-a.x, -a.y, -a.z);
	}

	public static implicit operator Vector3(Vector3i v) {
		return new Vector3(v.x, v.y, v.z);
	}

	public static Vector3 Zero => new Vector3(0f, 0f, 0f);
	public static Vector3 One => new Vector3(1f, 1f, 1f);
	public static Vector3 Up => new Vector3(0f, 1f, 0f);
	public static Vector3 Down => new Vector3(0f, -1f, 0f);
	public static Vector3 Left => new Vector3(-1f, 0f, 0f);
	public static Vector3 Right => new Vector3(1f, 0f, 0f);
	public static Vector3 Forward => new Vector3(0f, 0f, 1f);
	public static Vector3 Backward => new Vector3(0f, 0f, -1f);
}