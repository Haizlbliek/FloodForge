namespace FloodForge.World;

public class CurvedSlopeObject : LocalTerrainObject {
	public override string SaveKey => "CurvedSlope";

	public override CurvedSlopeObject Clone() {
		CurvedSlopeObject clone = new CurvedSlopeObject();
		DevObject.SetNodes(this, clone);
		clone.midpoints = [.. this.midpoints];
		return clone;
	}
}