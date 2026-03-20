namespace FloodForge.Utils;

public class Direction {
	public const uint Right = 0;
	public const uint Up = 1;
	public const uint Left = 2;
	public const uint Down = 3;
	public const uint Unknown = 4;

	public static readonly Vector2[] vectors = [ new Vector2(1, 0), new Vector2(0, 1), new Vector2(-1, 0), new Vector2(0, -1) ];
	public static Vector2 ToVector(uint direction) {
		return vectors[direction];
	}
}