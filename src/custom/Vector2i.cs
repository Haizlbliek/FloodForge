using System;
using System.Runtime.InteropServices;

namespace Custom;

[StructLayout(LayoutKind.Sequential)]
public struct Vector2i : IEquatable<Vector2i> {
	public int x;
	public int y;

	public Vector2i(int x, int y) {
		this.x = x;
		this.y = y;
	}

	public static Vector2i operator +(Vector2i a, Vector2i b) {
		return new Vector2i(a.x + b.x, a.y + b.y);
	}

	public static Vector2i operator -(Vector2i a, Vector2i b) {
		return new Vector2i(a.x - b.x, a.y - b.y);
	}

	public static Vector2i operator *(Vector2i a, Vector2i b) {
		return new Vector2i(a.x * b.x, a.y * b.y);
	}

	public static Vector2i operator /(Vector2i a, Vector2i b) {
		return new Vector2i(a.x / b.x, a.y / b.y);
	}

	public static Vector2i operator -(Vector2i a) {
		return new Vector2i(-a.x, -a.y);
	}

	public override readonly string ToString() {
		return $"({this.x}, {this.y})";
	}

	public override readonly bool Equals(object? obj) {
		return obj is Vector2i v && this.x == v.x && this.y == v.y;
	}

	public readonly bool Equals(Vector2i other) {
		return this.x == other.x && this.y == other.y;
	}

	public static bool operator ==(Vector2i left, Vector2i right) => left.Equals(right);
	public static bool operator !=(Vector2i left, Vector2i right) => !left.Equals(right);


	public static implicit operator Vector2(Vector2i v) {
		return new Vector2(v.x, v.y);
	}

	public override readonly int GetHashCode() {
		return HashCode.Combine(this.x, this.y);
	}

	public static Vector2i Zero => new Vector2i(0, 0);
	public static Vector2i One => new Vector2i(1, 1);
}