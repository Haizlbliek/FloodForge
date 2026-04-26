namespace FloodForge.World;

public class RoomPlacementVisualiser : WorldDraggable {
    public Vector2i size;
    public void Draw() {
        Immediate.Color(1f, 1f, 1f, 0.5f);
        Rect roomRect = new Rect(
            this.Position.x, this.Position.y - this.size.y,
            this.Position.x + this.size.x, this.Position.y
        );
        UI.FillRect(roomRect);
        Immediate.Color(Themes.Background);
        UI.StrokeRect(roomRect);
    }

	public bool Inside(Vector2 pos) {
		Vector2 position = this.Position;
		return pos.x >= position.x && pos.y >= position.y - this.size.y && pos.x < position.x + this.size.x && pos.y <= position.y;
	}
}