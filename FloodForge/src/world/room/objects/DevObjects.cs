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
}