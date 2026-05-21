namespace FloodForge.Droplet;

public class SuperSlopeObject : DevObject, ISaveableObject {
	public SuperSlopeObject() {
		this.AddNode(new Vector2(100f, 100f), this.nodes[0]);
		this.AddNode(new Vector2(0f, -20f), this.nodes[0]);
	}

	public string SaveKey => "SuperSlope";

	public override SuperSlopeObject Clone() {
		SuperSlopeObject clone = new SuperSlopeObject();
		DevObject.SetNodes(this, clone);
		return clone;
	}

	public override void Draw(Vector2 offset) {
		Immediate.Color(0.5f, 0.5f, 0.5f);
		this.nodes[2].position.x = 0f;
		this.nodes[2].position.y = MathF.Min(this.nodes[2].position.y, -10f);
		Vector2 pos = offset + this.nodes[0].position / 20f;
		UI.Line(pos, pos + this.nodes[1].position / 20f);
		UI.Line(pos, pos + this.nodes[2].position / 20f);
		UI.Line(pos + this.nodes[1].position / 20f, pos + this.nodes[2].position / 20f + this.nodes[1].position / 20f);
		UI.Line(pos + this.nodes[2].position / 20f, pos + this.nodes[2].position / 20f + this.nodes[1].position / 20f);
	}

	public void Load(Vector2 pos, string[] splits) {
		this.nodes[0].position = pos;
		if (splits.Length >= 3) {
			this.nodes[1].position = new Vector2(float.Parse(splits[0]), float.Parse(splits[1]));
			this.nodes[2].position.y = -float.Parse(splits[2]);
		}
	}

	public string Save(string[] splits) {
		return $"SuperSlope><{this.nodes[0].position.x}><{this.nodes[0].position.y}><{this.nodes[1].position.x}~{this.nodes[1].position.y}~{-this.nodes[2].position.y}";
	}
}