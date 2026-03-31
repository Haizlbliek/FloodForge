using System.Diagnostics.CodeAnalysis;

namespace Custom;

[StructLayout(LayoutKind.Sequential)]
public struct Color : IParsable<Color> {
	public float r;
	public float g;
	public float b;
	public float a = 1f;

	public float Average => (this.r + this.g + this.b) / 3f;
	public float Grayscale => 0.299f * this.r + 0.587f * this.g + 0.114f * this.b;

	public Color(float r, float g, float b, float a = 1.0f) {
		this.r = r;
		this.g = g;
		this.b = b;
		this.a = a;
	}

	public Color(float r, float g, float b) : this(r, g, b, 1.0f) { }

	public static Color operator +(Color a, Color b) {
		return new Color(a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a);
	}

	public static Color operator -(Color a, Color b) {
		return new Color(a.r - b.r, a.g - b.g, a.b - b.b, a.a - b.a);
	}

	public static Color operator *(Color a, float b) {
		return new Color(a.r * b, a.g * b, a.b * b, a.a * b);
	}

	public static Color operator /(Color a, float b) {
		return new Color(a.r / b, a.g / b, a.b / b, a.a / b);
	}

	private const string hex = "0123456789abcdef";
	private static string HashPart(float fv) {
		int v = Mathf.RoundToInt(fv * 255f);
		if (v < 16) {
			return "0" + hex[v];
		}
		return hex[v / 16].ToString() + hex[v % 16];
	}

	public readonly Color WithAlpha(float a) => new Color(this.r, this.g, this.b, a);

	public readonly Vector3 ToHsv() {
		ToHSV(this, out float h, out float s, out float v);
		return new Vector3(h, s, v);
	}

	public static void ToHSV(Color color, out float h, out float s, out float v) {
		float cmax = Math.Max(color.r, Math.Max(color.g, color.b));
		float cmin = Math.Min(color.r, Math.Min(color.g, color.b));
		float delta = cmax - cmin;

		h = 0.0f;
		s = 0.0f;
		v = cmax;

		if (delta > 0.0f) {
			if (cmax == color.r) {
				h = 60.0f * ((color.g - color.b) / delta % 6.0f);
			}
			else if (cmax == color.g) {
				h = 60.0f * (((color.b - color.r) / delta) + 2.0f);
			}
			else {
				h = 60.0f * (((color.r - color.g) / delta) + 4.0f);
			}
			if (h < 0.0f) h += 360.0f;
		}

		if (cmax > 0.0f) {
			s = delta / cmax;
		}
	}

	public static Color FromHSV(float h, float s, float v, float a = 1f) {
		float c = v * s;
		float x = c * (1.0f - Math.Abs(h / 60.0f % 2.0f - 1.0f));
		float m = v - c;


		float r1, g1, b1;
		if (h < 60) { r1 = c; g1 = x; b1 = 0; }
		else if (h < 120) { r1 = x; g1 = c; b1 = 0; }
		else if (h < 180) { r1 = 0; g1 = c; b1 = x; }
		else if (h < 240) { r1 = 0; g1 = x; b1 = c; }
		else if (h < 300) { r1 = x; g1 = 0; b1 = c; }
		else { r1 = c; g1 = 0; b1 = x; }

		return new Color(r1 + m, g1 + m, b1 + m, a);
	}

	public override readonly string ToString() {
		return $"#{HashPart(this.r)}{HashPart(this.g)}{HashPart(this.b)}{(this.a >= 1f ? "" : HashPart(this.a))}";
	}

	public static Color Lerp(Color a, Color b, float t) {
		return new Color(
			Mathf.Lerp(a.r, b.r, t),
			Mathf.Lerp(a.g, b.g, t),
			Mathf.Lerp(a.b, b.b, t),
			Mathf.Lerp(a.a, b.a, t)
		);
	}

	public static Color Clamp(Color color) {
		return new Color(
			Mathf.Clamp(color.r, 0.0f, 1.0f),
			Mathf.Clamp(color.g, 0.0f, 1.0f),
			Mathf.Clamp(color.b, 0.0f, 1.0f),
			Mathf.Clamp(color.a, 0.0f, 1.0f)
		);
	}

	public static float ParsePart(string value) {
		return (hex.IndexOf(value[0]) * 16f + hex.IndexOf(value[1])) / 255f;
	}

	public static Color Parse(string s, IFormatProvider? provider = null) {
		s = s.ToLowerInvariant();
		if (s.StartsWith("#")) s = s[1..];

		return new Color(
			ParsePart(s[0..2]),
			ParsePart(s[2..4]),
			ParsePart(s[4..6]),
			s.Length <= 6 ? 1f : ParsePart(s[6..8])
		);
	}

	public static bool TryParse([NotNullWhen(true)] string? s, IFormatProvider? provider, [MaybeNullWhen(false)] out Color result) {
		result = default;
		if (string.IsNullOrWhiteSpace(s)) return false;
		try {
			result = Parse(s, provider);
			return true;
		} catch {
			return false;
		}
	}

	public static Color Black => new Color(0.0f, 0.0f, 0.0f, 1.0f);
	public static Color Grey => new Color(0.5f, 0.5f, 0.5f, 1.0f);
	public static Color White => new Color(1.0f, 1.0f, 1.0f, 1.0f);
	public static Color Red => new Color(1.0f, 0.0f, 0.0f, 1.0f);
	public static Color Green => new Color(0.0f, 1.0f, 0.0f, 1.0f);
	public static Color Blue => new Color(0.0f, 0.0f, 1.0f, 1.0f);
	public static Color Transparent => new Color(0.0f, 0.0f, 0.0f, 0.0f);
	public static Color Yellow => new Color(1.0f, 1.0f, 0.0f, 1.0f);
	public static Color Cyan => new Color(0.0f, 1.0f, 1.0f, 1.0f);
	public static Color Magenta => new Color(1.0f, 0.0f, 1.0f, 1.0f);
}