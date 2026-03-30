namespace FloodForge.World;

public class GenericItemObject : DevObject {
	public string name;
	public Texture texture;

	public override bool ShowInDroplet => false;

	public GenericItemObject(string name, Texture texture) {
		this.name = name;
		this.texture = texture;
	}

	public override GenericItemObject Clone() {
		GenericItemObject clone = new GenericItemObject(this.name, this.texture);
		DevObject.SetNodes(this, clone);
		return clone;
	}

	public override void Draw(Vector2 offset) {
		UI.CenteredTexture(
			this.texture,
			offset.x + this.nodes[0].position.x / 20f,
			offset.y + this.nodes[0].position.y / 20f,
			Main.mode == Main.Mode.World ? WorldWindow.SelectorScale : 1f
		);
	}
}