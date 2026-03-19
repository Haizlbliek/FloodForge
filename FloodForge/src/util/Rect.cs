namespace FloodForge.Utils;

public struct Rect {
	public float x0, y0, x1, y1;

	public readonly float CenterX => (this.x0 + this.x1) * 0.5f;
	public readonly float CenterY => (this.y0 + this.y1) * 0.5f;

	public Rect() {
		this.x0 = this.y0 = this.x1 = this.y1 = 0f;
	}

	public Rect(UVRect rect) {
		this.x0 = rect.x0;
		this.y0 = rect.y0;
		this.x1 = rect.x1;
		this.y1 = rect.y1;
	}

	public Rect(float x0, float y0, float x1, float y1) {
		this.x0 = MathF.Min(x0, x1);
		this.y0 = MathF.Min(y0, y1);
		this.x1 = MathF.Max(x0, x1);
		this.y1 = MathF.Max(y0, y1);
	}

	public Rect(Vector2 a, Vector2 b) {
		this.x0 = MathF.Min(a.x, b.x);
		this.y0 = MathF.Min(a.y, b.y);
		this.x1 = MathF.Max(a.x, b.x);
		this.y1 = MathF.Max(a.y, b.y);
	}

	public readonly bool Inside(Vector2 point) {
		return this.Inside(point.x, point.y);
	}

	public readonly bool Inside(float x, float y) {
		return x >= this.x0 && y >= this.y0 && x <= this.x1 && y <= this.y1;
	}

	public static Rect operator +(Rect left, Vector2 right) {
		return new Rect(left.x0 + right.x, left.y0 + right.y, left.x1 + right.x, left.y1 + right.y);
	}

	public static Rect FromSize(float x, float y, float width, float height) {
		return new Rect(x, y, x + width, y + height);
	}

	public static Rect FromSize(Vector2 pos, Vector2 size) {
		return new Rect(pos.x, pos.y, pos.x + size.x, pos.y + size.y);
	}
}