namespace FloodForge.World;

public abstract class DevObject {
	public readonly List<Node> nodes = [ ];

	public virtual bool ShowInDroplet => true;

	public Node AddNode(Vector2 pos, Node? parent = null) {
		Node node = new Node(pos, parent, this);
		this.nodes.Add(node);
		return node;
	}

	public DevObject() {
		this.AddNode(Vector2.Zero);
	}

	public abstract void Draw(Vector2 offset);

	public static void SetNodes<T>(T from, T to) where T : DevObject {
		to.nodes.Clear();
		foreach (Node node in from.nodes) {
			to.nodes.Add(new Node(node.position, null, to));
		}
		for (int i = 0; i < from.nodes.Count; i++) {
			if (from.nodes[i].parent == null) continue;

			to.nodes[i].parent = to.nodes[from.nodes.IndexOf(from.nodes[i].parent!)];
		}
	}

	public abstract DevObject Clone();
}