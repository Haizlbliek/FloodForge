namespace FloodForge.Utils;

public struct UVRect {
	public float x0, y0, x1, y1;
	public Vector2 uv0, uv1, uv2, uv3;

	public readonly float CenterX => (this.x0 + this.x1) * 0.5f;
	public readonly float CenterY => (this.y0 + this.y1) * 0.5f;

	public UVRect() {
		this.x0 = this.y0 = this.x1 = this.y1 = 0f;
		this.uv0 = new Vector2(0f, 1f);
		this.uv1 = new Vector2(1f, 1f);
		this.uv2 = new Vector2(1f, 0f);
		this.uv3 = new Vector2(0f, 0f);
	}

	public UVRect(float x0, float y0, float x1, float y1) {
		this.x0 = x0;
		this.y0 = y0;
		this.x1 = x1;
		this.y1 = y1;
		this.uv0 = new Vector2(0f, 1f);
		this.uv1 = new Vector2(1f, 1f);
		this.uv2 = new Vector2(1f, 0f);
		this.uv3 = new Vector2(0f, 0f);
	}

	public UVRect(Vector2 a, Vector2 b) {
		this.x0 = MathF.Min(a.x, b.x);
		this.y0 = MathF.Min(a.y, b.y);
		this.x1 = MathF.Max(a.x, b.x);
		this.y1 = MathF.Max(a.y, b.y);
		this.uv0 = new Vector2(0f, 1f);
		this.uv1 = new Vector2(1f, 1f);
		this.uv2 = new Vector2(1f, 0f);
		this.uv3 = new Vector2(0f, 0f);
	}

	public UVRect UV(float u0, float v0, float u1, float v1) {
		this.uv0 = new Vector2(u0, v0);
		this.uv1 = new Vector2(u1, v0);
		this.uv2 = new Vector2(u1, v1);
		this.uv3 = new Vector2(u0, v1);

		return this;
	}

	public readonly bool Inside(Vector2 point) {
		return this.Inside(point.x, point.y);
	}

	public readonly bool Inside(float x, float y) {
		return x >= this.x0 && y >= this.y0 && x <= this.x1 && y <= this.y1;
	}

	public static UVRect operator +(UVRect left, Vector2 right) {
		return new UVRect(left.x0 + right.x, left.y0 + right.y, left.x1 + right.x, left.y1 + right.y);
	}

	public static UVRect FromSize(float x, float y, float width, float height) {
		return new UVRect(x, y, x + width, y + height);
	}

	public static UVRect FromSize(Vector2 pos, Vector2 size) {
		return new UVRect(pos.x, pos.y, pos.x + size.x, pos.y + size.y);
	}
}