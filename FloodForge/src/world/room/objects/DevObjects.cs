namespace FloodForge.World;

public static class DevObjects {
	public static readonly Dictionary<string, Func<DevObject>> objectFactories = new Dictionary<string, Func<DevObject>> {
		{ "TerrainHandle", () => new TerrainHandleObject() },
		{ "MudPit",        () => new MudPitObject() },
		{ "AirPocket",     () => new AirPocketObject() },
		{ "SuperSlope",    () => new SuperSlopeObject() },
		{ "CurvedSlope",   () => new CurvedSlopeObject() },
		{ "LocalTerrain",  () => new LocalTerrainObject() },
	};

	public static readonly Color TerrainColor = new Color(0.1f, 0.8f, 0.3f);
	public static readonly Color HeightColor = new Color(0.2f, 0.2f, 0.5f);
	public static readonly Color MainHandleColor = new Color(0.6f, 0.65f, 0.7f);
}