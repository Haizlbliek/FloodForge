using System.Runtime.CompilerServices;

namespace Custom;

public static class Noise {
	public static float Random3(float x, float y, float z) {
		uint ux = Unsafe.As<float, uint>(ref x);
		uint uy = Unsafe.As<float, uint>(ref y);
		uint uz = Unsafe.As<float, uint>(ref z);

		uint hash = ux;
		hash ^= uy;
		hash ^= uz;
		hash ^= hash >> 16;
		hash *= 0x85ebca6bu;
		hash ^= hash >> 13;
		hash *= 0xc2b2ae35u;
		hash ^= hash >> 16;

		return (float) hash / uint.MaxValue;
	}

	public static float DotGridGradient3(float ix, float iy, float iz, float x, float y, float z) {
		float random = Random3(ix, iy, iz);

		float theta = 2f * Mathf.PI * random;
		float phi = MathF.Acos(1f - 2f * random);

		return (
			(x - ix) * Mathf.Sin(phi) * Mathf.Cos(theta) +
			(y - iy) * Mathf.Cos(phi) +
			(z - iz) * Mathf.Sin(phi) * Mathf.Sin(theta)
		);
	}

	public static float Perlin3(float x, float y, float z) {
		float x0 = MathF.Floor(x);
		float x1 = x0 + 1f;
		float y0 = MathF.Floor(y);
		float y1 = y0 + 1f;
		float z0 = MathF.Floor(z);
		float z1 = z0 + 1f;

		float sx = x - x0;
		float sy = y - y0;
		float sz = z - z0;

		float ix0, ix1;

		ix0 = Mathf.Lerp(
			DotGridGradient3(x0, y0, z0, x, y, z),
			DotGridGradient3(x1, y0, z0, x, y, z),
			sx
		);
		ix1 = Mathf.Lerp(
			DotGridGradient3(x0, y1, z0, x, y, z),
			DotGridGradient3(x1, y1, z0, x, y, z),
			sx
		);
		float ty0 = Mathf.Lerp(ix0, ix1, sy);

		ix0 = Mathf.Lerp(
			DotGridGradient3(x0, y0, z1, x, y, z),
			DotGridGradient3(x1, y0, z1, x, y, z),
			sx
		);
		ix1 = Mathf.Lerp(
			DotGridGradient3(x0, y1, z1, x, y, z),
			DotGridGradient3(x1, y1, z1, x, y, z),
			sx
		);
		float ty1 = Mathf.Lerp(ix0, ix1, sy);

		return Mathf.Lerp(ty0, ty1, sz);
	}
}