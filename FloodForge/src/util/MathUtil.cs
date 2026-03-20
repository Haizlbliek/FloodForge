namespace FloodForge.Utils;

public static class MathUtil {
	public static Vector2 BezierCubic(float t, Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3) {
		float u = 1 - t;
		return p0 * u * u * u + p1 * t * u * u * 3f + p2 * t * t * u * 3f + p3 * t * t * t;
	}

	public static float LineDistance(Vector2 p, Vector2 a, Vector2 b) {
		Vector2 ab = b - a;
		Vector2 ap = p - a;
		float t = (ap.x * ab.x + ap.y * ab.y) / ab.SqrLength;
		t = Mathf.Clamp(t, 0f, 1f);

		return (a + ab * t - p).Length;
	}
}