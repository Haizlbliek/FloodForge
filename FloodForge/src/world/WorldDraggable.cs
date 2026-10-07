namespace FloodForge.World;

public class WorldDraggable {
	public bool Visible => this.IsVisible();
	protected virtual bool IsVisible() {
		return true;
	}

	public bool Selectable => this.IsSelectable();
	protected virtual bool IsSelectable() {
		return this.Visible;
	}

	public bool Draggable => this.IsDraggable();
	protected virtual bool IsDraggable() {
		return this.Visible;
	}

	protected Vector2 position;
	public Vector2 Position {
		get {
			return this.GetPosition();
		}
		set {
			this.SetPosition(value);
		}
	}

	public virtual Vector2 GetPosition() {
		return this.position;
	}

	public virtual void SetPosition(Vector2 value) {
		this.position = value;
	}
}