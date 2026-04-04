namespace FloodForge.World;

public class OffscreenRoom : Room {
	public OffscreenRoom(string path, string name) : base(path, name) {
	}

	public Den GetDen() {
		if (this.dens.Count == 0) {
			this.dens.Add(new Den());
			this.denShortcutEntrances.Add(new Vector2i(0, 0));
		}

		return this.dens[0];
	}

	protected override void GenerateMesh() {
	}

	protected override void LoadGeometry() {
		this.width = 72;
		this.height = 43;
		this.valid = true;
		this.data.tags.Add("OffscreenRoom");
	}

	protected override void LoadSettings() {
	}

	public override void DrawBlack(WorldWindow.RoomPosition positionType) {
		if (!WorldWindow.VisibleCreatures) return;

		base.DrawBlack(positionType);
	}

	public override void Draw(WorldWindow.RoomPosition positionType) {
		if (!WorldWindow.VisibleCreatures) return;

		Vector2 position = positionType == WorldWindow.RoomPosition.Canon ? this.CanonPosition : this.DevPosition;

		Immediate.Color(Themes.RoomAir);
		UI.FillRect(position.x, position.y, position.x + this.width, position.y - this.height);

		Immediate.Color(Themes.RoomSolid);
		UI.font.Write(this.name, position.x + (this.width * 0.5f), position.y - (this.height * 0.5f), 5f, Font.Align.MiddleCenter);

		if (this.dens.Count == 0) {
			this.dens.Add(new Den());
			this.denShortcutEntrances.Add(new Vector2i(0, 0));
		}

		this.DrawDen(this.dens[0], position.x + this.width * 0.5f, position.y - this.height * 0.25f, 0 == this.hoveredDen, WorldWindow.HoveringRoom == this);

		Vector2 o = WorldWindow.worldMouse - position;
		bool hovered = o.x >= 0f && o.y >= 0f && o.x <= this.width && o.y <= this.height;
		Immediate.Color(hovered ? Themes.RoomBorderHighlight : Themes.RoomBorder);
		UI.StrokeRect(position.x, position.y, position.x + this.width, position.y - this.height);
	}
}