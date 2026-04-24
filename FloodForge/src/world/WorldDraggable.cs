namespace FloodForge.World;

public class WorldDraggable {
	public bool Visible => this.IsVisible();
    public virtual bool IsVisible() {
        return true;
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