using System.Runtime.InteropServices;

namespace Custom;

[StructLayout(LayoutKind.Sequential)]
public struct Vector3i {
	public int x;
	public int y;
	public int z;

	public Vector3i(int x, int y, int z) {
		this.x = x;
		this.y = y;
		this.z = z;
	}

	public static Vector3i operator +(Vector3i a, Vector3i b) {
		return new Vector3i(a.x + b.x, a.y + b.y, a.z + b.z);
	}

	public static Vector3i operator -(Vector3i a, Vector3i b) {
		return new Vector3i(a.x - b.x, a.y - b.y, a.z - b.z);
	}

	public static Vector3i operator *(Vector3i a, Vector3i b) {
		return new Vector3i(a.x * b.x, a.y * b.y, a.z * b.z);
	}

	public static Vector3i operator /(Vector3i a, Vector3i b) {
		return new Vector3i(a.x / b.x, a.y / b.y, a.z / b.z);
	}

	public static Vector3i operator *(Vector3i a, int b) {
		return new Vector3i(a.x * b, a.y * b, a.z * b);
	}

	public static Vector3i operator /(Vector3i a, int b) {
		return new Vector3i(a.x / b, a.y / b, a.z / b);
	}

	public static Vector3i operator -(Vector3i a) {
		return new Vector3i(-a.x, -a.y, -a.z);
	}

	public static Vector3i Zero => new Vector3i(0, 0, 0);
	public static Vector3i One => new Vector3i(1, 1, 1);
	public static Vector3i Up => new Vector3i(0, 1, 0);
	public static Vector3i Down => new Vector3i(0, -1, 0);
	public static Vector3i Left => new Vector3i(-1, 0, 0);
	public static Vector3i Right => new Vector3i(1, 0, 0);
	public static Vector3i Forward => new Vector3i(0, 0, 1);
	public static Vector3i Backward => new Vector3i(0, 0, -1);
}