namespace FloodForge.World;

public class AirPocketObject : DevObject, ISaveableObject {
	public AirPocketObject() {
		this.AddNode(new Vector2(100f, 200f), this.nodes[0]);
		this.AddNode(new Vector2(0f, 80f), this.nodes[0]);
	}

	public string SaveKey => "AirPocket";

	public override AirPocketObject Clone() {
		AirPocketObject clone = new AirPocketObject();
		DevObject.SetNodes(this, clone);
		return clone;
	}

	public override void Draw(Vector2 offset) {
		Immediate.Color(0f, 0f, 1f);
		UI.StrokeRect(Rect.FromSize(
			offset + this.nodes[0].position / 20f,
			this.nodes[1].position / 20f
		));

		Immediate.Color(0f, 1f, 1f);
		UI.Line(
			offset.x + this.nodes[0].position.x / 20f,
			offset.y + this.nodes[0].position.y / 20f + this.nodes[2].position.y / 20f,
			offset.x + this.nodes[0].position.x / 20f + this.nodes[1].position.x / 20f,
			offset.y + this.nodes[0].position.y / 20f + this.nodes[2].position.y / 20f
		);
		this.nodes[2].position.x = 0f;
	}

	public void Load(Vector2 pos, string[] splits) {
		this.nodes[0].position = pos;
		if (splits.Length >= 6) {
			this.nodes[1].position = new Vector2(float.Parse(splits[0]), float.Parse(splits[1]));
			this.nodes[2].position.y = float.Parse(splits[5]);
		}
	}

	public string Save(string[] splits) {
		string px = "30.0", py = "30.0", flood = "Y";
		if (splits.Length >= 6) { px = splits[2]; py = splits[3]; flood = splits[4]; }
		return $"AirPocket><{this.nodes[0].position.x}><{this.nodes[0].position.y}><{this.nodes[1].position.x}~{this.nodes[1].position.y}~{px}~{py}~{flood}~{this.nodes[2].position.y}";
	}
}