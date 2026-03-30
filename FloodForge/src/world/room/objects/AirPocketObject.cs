namespace FloodForge.World;

public class AirPocketObject : DevObject {
	public AirPocketObject() {
		this.AddNode(new Vector2(100f, 200f), this.nodes[0]);
		this.AddNode(new Vector2(0f, 80f), this.nodes[0]);
	}

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
}