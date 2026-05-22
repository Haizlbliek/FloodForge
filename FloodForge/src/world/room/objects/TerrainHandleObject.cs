namespace FloodForge.World;

public class TerrainHandleObject : DevObject, ISaveableObject {
	public Vector2 Left => this.nodes[0].position + this.nodes[1].position;
	public Vector2 Middle => this.nodes[0].position;
	public Vector2 Right => this.nodes[0].position + this.nodes[2].position;

	public TerrainHandleObject() {
		this.AddNode(new Vector2(-40f, 0f), this.nodes[0], color: DevObjects.TerrainColor);
		this.AddNode(new Vector2(40f, 0f), this.nodes[0], color: DevObjects.TerrainColor);
	}

	public string SaveKey => "TerrainHandle";

	public override TerrainHandleObject Clone() {
		TerrainHandleObject clone = new TerrainHandleObject();
		DevObject.SetNodes(this, clone);
		return clone;
	}

	public override void Draw(Vector2 offset) {
		Immediate.Color(Color.Grey);
		UI.Line(
			offset.x + this.nodes[0].position.x / 20f,
			offset.y + this.nodes[0].position.y / 20f,
			offset.x + this.nodes[0].position.x / 20f + this.nodes[1].position.x / 20f,
			offset.y + this.nodes[0].position.y / 20f + this.nodes[1].position.y / 20f
		);
		UI.Line(
			offset.x + this.nodes[0].position.x / 20f,
			offset.y + this.nodes[0].position.y / 20f,
			offset.x + this.nodes[0].position.x / 20f + this.nodes[2].position.x / 20f,
			offset.y + this.nodes[0].position.y / 20f + this.nodes[2].position.y / 20f
		);
	}

	public void Load(Vector2 pos, string[] splits) {
		this.nodes[0].position = pos;
		if (splits.Length >= 4) {
			this.nodes[1].position = new Vector2(float.Parse(splits[0]), float.Parse(splits[1]));
			this.nodes[2].position = new Vector2(float.Parse(splits[2]), float.Parse(splits[3]));
		}
	}

	public string Save(string[] splits) {
		string height = splits.Length >= 5 ? splits[4] : "20";
		return $"TerrainHandle><{this.nodes[0].position.x}><{this.nodes[0].position.y}><{this.nodes[1].position.x}~{this.nodes[1].position.y}~{this.nodes[2].position.x}~{this.nodes[2].position.y}~{height}";
	}
}