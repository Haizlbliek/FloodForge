namespace FloodForge.World;

public sealed class Node {
	public Vector2 position;
	public Node? parent;
	public readonly DevObject devObject;

	public Vector2 GlobalPosition => this.parent == null ? this.position : (this.position + this.parent.GlobalPosition);

	public Node(Vector2 pos, Node? parent, DevObject devObject) {
		this.position = pos;
		this.parent = parent;
		this.devObject = devObject;
	}
}