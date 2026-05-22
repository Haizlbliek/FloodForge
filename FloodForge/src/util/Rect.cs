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

	public readonly bool IntersectsLine(Vector2 from, Vector2 to) {
		if (this.Inside(from) || this.Inside(to))
			return true;
		
		/*
		Immediate.Color(new Color(1, 1, 0));
		UI.Line(from, to, thickness: 0.25f * WorldWindow.cameraScale);
		Immediate.Color(new Color(1, 0, 0));
		UI.StrokeCircle(from, 0.5f, 8);
		Immediate.Color(new Color(0, 1, 0));
		UI.StrokeCircle(to, 0.5f, 8);
		*/

		float xDelta = Math.Abs(from.x - to.x);
		float yDelta = Math.Abs(from.y - to.y);
		if (!(from.x == to.x) && ((this.x1 < from.x && this.x1 > to.x) || (this.x0 > from.x && this.x0 < to.x))) {
			Vector2 startPoint = from.x < to.x ? from : to;
			float xRelative = (this.x0 > from.x && this.x0 < to.x ? this.x0 : this.x1 ) - startPoint.x; // relative X position of the concerning box edge line
			float xYDelta = xRelative * (yDelta / xDelta); // relative Y position when continuing across the line to the same X position
			float theoreticalIntersectY = startPoint.y + xYDelta; // absolute Y position at box edge line X
			if (this.y0 < theoreticalIntersectY && this.y1 > theoreticalIntersectY) // if the point on the line is within the box, there's an intersection
				return true;
		}
		if (!(from.y == to.y) && ((this.y1 < from.y && this.y1 > to.y) || (this.y0 > from.y && this.y0 < to.y))) {
			Vector2 startPoint = from.y < to.y ? from : to;
			float yRelative = (this.y0 > from.y && this.y0 < to.y ? this.y0 : this.y1 ) - startPoint.y; // relative Y position of the concerning box edge line
			float yXDelta = yRelative * (xDelta / yDelta); // relative X position when continuing across the line to the same Y position
			float theoreticalIntersectX = startPoint.x + yXDelta; // absolute X position at box edge line Y
			if (this.x0 < theoreticalIntersectX && this.x1 > theoreticalIntersectX) // if the point on the line is within the box, there's an intersection
				return true;
		}

		return false;
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