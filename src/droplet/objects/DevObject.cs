namespace FloodForge.Droplet;

public abstract class DevObject {
	public List<Node> nodes = [ ];

	public void AddNode(Vector2 pos, Node? parent = null) {
		this.nodes.Add(new Node(pos, parent, this));
	}

	public DevObject() {
		this.AddNode(Vector2.Zero);
	}

	public abstract void Draw(Vector2 offset);
}