namespace FloodForge.Droplet;

public class TerrainHandleObject : DevObject {
	public Vector2 Left => this.nodes[0].position + this.nodes[1].position;
	public Vector2 Middle => this.nodes[0].position;
	public Vector2 Right => this.nodes[0].position + this.nodes[2].position;

	public TerrainHandleObject() {
		this.AddNode(new Vector2(-40f, 0f), this.nodes[0]);
		this.AddNode(new Vector2(40f, 0f), this.nodes[0]);
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
}